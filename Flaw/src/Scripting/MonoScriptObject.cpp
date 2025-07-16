#include "pch.h"
#include "MonoScriptObject.h"
#include "MonoScriptClass.h"
#include "MonoScriptClassField.h"
#include "Log/Log.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-debug.h>
#include <mono/metadata/threads.h>
#include <fmt/format.h>

namespace flaw {
	MonoScriptObjectView::MonoScriptObjectView(MonoScriptClass& clss, MonoObject* obj)
		: _domain(clss.GetMonoDomain())
		, _clss(clss.GetMonoClass())
		, _obj(obj)
	{
	}

	MonoScriptClass MonoScriptObjectView::GetClass() const {
		return MonoScriptClass(_domain, _clss);
	}

	MonoObject* MonoScriptObjectView::GetMonoObject() const {
		return _obj;
	}

	MonoScriptObjectView::operator bool() const {
		return _obj;
	}

	MonoScriptObject::MonoScriptObject()
		: _domain(nullptr)
		, _clss(nullptr)
		, _gcHandle(0)
	{
	}

	MonoScriptObject::MonoScriptObject(MonoScriptClass& clss)
		: _domain(clss.GetMonoDomain())
		, _clss(clss.GetMonoClass())
		, _gcHandle(0)
	{
	}

	MonoScriptObject::MonoScriptObject(MonoScriptObject&& other) noexcept 
		: _domain(other._domain)
		, _clss(other._clss)
		, _gcHandle(other._gcHandle)
	{
		other._gcHandle = 0;
	}

	MonoScriptObject& MonoScriptObject::operator=(MonoScriptObject&& other) noexcept {
		if (this != &other) {
			GcHandleFree();

			_domain = other._domain;
			_clss = other._clss;
			_gcHandle = other._gcHandle;
			other._gcHandle = 0;
		}

		return *this;
	}

	MonoScriptObject::~MonoScriptObject() {
		GcHandleFree();
	}

	void MonoScriptObject::InstantiateDefaultConstructorImpl() {
		MonoObject* obj = mono_object_new(_domain, _clss);
		if (!obj) {
			throw std::runtime_error("mono_object_new failed");
		}
		mono_runtime_object_init(obj);

		GcHandleFree();
		GcHandleNew(obj);
	}

	void MonoScriptObject::InstantiateImpl(void** constructorArgs, int32_t argCount) {
		MonoClass* clss = _clss;
		MonoMethod* ctor = mono_class_get_method_from_name(clss, ".ctor", argCount);

		while (!ctor) {
			clss = mono_class_get_parent(clss);
			if (!clss) {
				break;
			}
			ctor = mono_class_get_method_from_name(clss, ".ctor", argCount);
		}

		if (!ctor) {
			throw std::runtime_error("Constructor not found");
		}

		MonoObject* obj = mono_object_new(_domain, _clss);
		if (!obj) {
			throw std::runtime_error("mono_object_new failed");
		}

		MonoObject* exception = nullptr;
		mono_runtime_invoke(ctor, obj, constructorArgs, &exception);
		if (exception) {
			throw std::runtime_error("Exception during object instantiation");
		}

		GcHandleFree();
		GcHandleNew(obj);
	}

	void MonoScriptObject::CallMethodImpl(MonoMethod* method, void** args, int32_t argCount) const {
		MonoObject* exception = nullptr;
		MonoObject* result = mono_runtime_invoke(method, GetMonoObject(), args, &exception);

		if (exception) {
			MonoString* excStr = mono_object_to_string(exception, nullptr);
			const char* excCStr = mono_string_to_utf8(excStr);
			Log::Error("Exception during method invocation: %s", excCStr);
			mono_free((void*)excCStr);
		}
	}

	void MonoScriptObject::DeepCloneFields(MonoObject* source, MonoObject* target, uint32_t expectedFieldsFlags, std::unordered_map<MonoObject*, MonoObject*>& clonedObjs) const {
		MonoClass* parentClass = mono_object_get_class(source);

		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(parentClass, &iter)) {
			const uint32_t fieldFlags = mono_field_get_flags(field);
			if (!(expectedFieldsFlags & fieldFlags)) {
				continue;
			}

			const char* fieldName = mono_field_get_name(field);
			MonoType* fieldType = mono_field_get_type(field);
			MonoClass* fieldClass = mono_class_from_mono_type(fieldType);

			MonoClassField* targetField = mono_class_get_field_from_name(parentClass, fieldName);
			
			int32_t typeType = mono_type_get_type(fieldType);
			if (typeType == MONO_TYPE_CLASS) {
				MonoObject* sourceFieldValue = nullptr;
				mono_field_get_value(source, field, &sourceFieldValue);
				if (!sourceFieldValue) {
					mono_field_set_value(target, targetField, nullptr);
					continue;
				}

				auto it = clonedObjs.find(sourceFieldValue);
				if (it != clonedObjs.end()) {
					// If the object has already been cloned, use the cloned version
					mono_field_set_value(target, targetField, it->second);
					continue;
				}

				MonoObject* targetFieldValue = mono_object_new(_domain, fieldClass);
				mono_runtime_object_init(targetFieldValue);
				mono_field_set_value(target, targetField, targetFieldValue);

				clonedObjs[sourceFieldValue] = targetFieldValue;

				DeepCloneFields(sourceFieldValue, targetFieldValue, expectedFieldsFlags, clonedObjs);
			}
			// TODO: handle struct, array, string near future
			else if(typeType == MONO_TYPE_VALUETYPE) {
				size_t size = mono_type_size(fieldType, nullptr);
				std::vector<int8_t> buff(size);
				mono_field_get_value(source, field, buff.data());
				mono_field_set_value(target, targetField, buff.data());
			}
		}
	}

	MonoScriptObject MonoScriptObject::Clone(uint32_t fieldFlags) const {
		MonoScriptObject clone(GetClass());
		clone.Instantiate();
		std::unordered_map<MonoObject*, MonoObject*> clonedObjs;
		clonedObjs[GetMonoObject()] = clone.GetMonoObject();
		DeepCloneFields(GetMonoObject(), clone.GetMonoObject(), fieldFlags, clonedObjs);
		return clone;
	}

	void MonoScriptObject::CreateTreeNodeRecurcive(MonoObject* obj, Ref<MonoScriptObjectTreeNodeObject> node) const {
		MonoClass* clss = mono_object_get_class(obj);
		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(clss, &iter)) {
			const char* fieldName = mono_field_get_name(field);
			MonoType* fieldType = mono_field_get_type(field);
			MonoClass* fieldClass = mono_class_from_mono_type(fieldType);

			if (mono_type_get_type(fieldType) == MONO_TYPE_CLASS) {
				MonoObject* fieldValue = nullptr;
				mono_field_get_value(obj, field, &fieldValue);
				if (!fieldValue) {
					continue;
				}
				auto elmNode = CreateRef<MonoScriptObjectTreeNodeObject>();
				elmNode->name = fieldName;
				elmNode->typeName = mono_type_get_name_full(fieldType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
				CreateTreeNodeRecurcive(fieldValue, elmNode);
				node->elements.push_back(elmNode);
			}
			else {
				auto valueNode = CreateRef<MonoScriptObjectTreeNodeValue>();
				valueNode->name = fieldName;
				valueNode->typeName = mono_type_get_name_full(fieldType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
				valueNode->valueData.resize(mono_class_instance_size(fieldClass) - sizeof(MonoObject));
				mono_field_set_value(obj, field, valueNode->valueData.data());
				node->elements.push_back(valueNode);
			}
		}
	}

	Ref<MonoScriptObjectTreeNode> MonoScriptObject::ToTree() const {
		MonoType* monoType = mono_class_get_type(_clss);
		bool isClass = mono_type_get_type(monoType) == MONO_TYPE_CLASS;

		Ref<MonoScriptObjectTreeNode> rootNode = CreateRef<MonoScriptObjectTreeNodeObject>();

		if (isClass) {
			auto objNode = CreateRef<MonoScriptObjectTreeNodeObject>();
			objNode->name = "";
			objNode->typeName = mono_type_get_name_full(monoType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
			CreateTreeNodeRecurcive(GetMonoObject(), objNode);
			rootNode = objNode;
		}
		else {
			auto valueNode = CreateRef<MonoScriptObjectTreeNodeValue>();
			valueNode->name = "";
			valueNode->typeName = mono_type_get_name_full(monoType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);

			size_t typeSize = mono_type_size(monoType, nullptr);
			valueNode->valueData.resize(typeSize);
			void* data = mono_object_unbox(GetMonoObject());
			std::memcpy(valueNode->valueData.data(), data, valueNode->valueData.size());
			rootNode = valueNode;
		}

		return rootNode;
	}

	void MonoScriptObject::ApplyTreeNodeRecurcive(MonoObject* obj, Ref<MonoScriptObjectTreeNodeObject> node) {
		for (const auto& child : node->elements) {
			MonoClass* clss = mono_object_get_class(obj);
			MonoClassField* field = mono_class_get_field_from_name(clss, child->name.c_str());
			if (!field) {
				continue;
			}

			MonoType* fieldType = mono_field_get_type(field);
			MonoClass* fieldClass = mono_class_from_mono_type(fieldType);
			if (child->typeName != mono_class_get_name(fieldClass)) {
				continue;
			}

			if (auto objectNode = std::dynamic_pointer_cast<MonoScriptObjectTreeNodeObject>(child)) {
				MonoObject* objPtr = nullptr;
				mono_field_get_value(objPtr, field, &objPtr);
				if (!objPtr) {
					objPtr = mono_object_new(_domain, fieldClass);
					mono_runtime_object_init(objPtr);
					mono_field_set_value(obj, field, objPtr);
				}
				ApplyTreeNodeRecurcive(objPtr, objectNode);
			}
			else if (auto valueNode = std::dynamic_pointer_cast<MonoScriptObjectTreeNodeValue>(child)) {
				uint32_t typeSize = mono_type_size(fieldType, nullptr);
				if (typeSize != valueNode->valueData.size()) {
					continue;
				}
				mono_field_set_value(obj, field, valueNode->valueData.data());
			}
		}
	}

	void MonoScriptObject::ApplyTree(const Ref<MonoScriptObjectTreeNode>& treeNode) {
		bool isClass = mono_type_get_type(mono_class_get_type(_clss)) == MONO_TYPE_CLASS;

		if (isClass) {
			auto objNode = std::dynamic_pointer_cast<MonoScriptObjectTreeNodeObject>(treeNode);
			if (!objNode) {
				throw std::runtime_error("Invalid tree node type for class object");
			}
			ApplyTreeNodeRecurcive(GetMonoObject(), objNode);
		}
		else {
			auto valueNode = std::dynamic_pointer_cast<MonoScriptObjectTreeNodeValue>(treeNode);
			if (!valueNode) {
				throw std::runtime_error("Invalid tree node type for value object");
			}
			if (valueNode->valueData.size() != mono_class_instance_size(_clss) - sizeof(MonoObject)) {
				throw std::runtime_error("Value data size mismatch");
			}
			void* data = mono_object_unbox(GetMonoObject());
			std::memcpy(data, valueNode->valueData.data(), valueNode->valueData.size());
		}
	}

	MonoScriptClass MonoScriptObject::GetClass() const {
		return MonoScriptClass(_domain, _clss);
	}

	MonoScriptObjectView MonoScriptObject::GetView() const {
		return MonoScriptObjectView(GetClass(), GetMonoObject());
	}

	bool MonoScriptObject::IsValid() const {
		return _gcHandle != 0;
	}

	MonoObject* MonoScriptObject::GetMonoObject() const {
		return mono_gchandle_get_target(_gcHandle);
	}

	void MonoScriptObject::GcHandleNew(MonoObject* obj) {
		_gcHandle = mono_gchandle_new(obj, false);
	}

	void MonoScriptObject::GcHandleFree() {
		if (_gcHandle != 0) {
			mono_gchandle_free(_gcHandle);
			_gcHandle = 0;
		}
	}
}