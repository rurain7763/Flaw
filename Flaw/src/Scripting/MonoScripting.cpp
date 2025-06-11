#include "pch.h"
#include "MonoScripting.h"
#include "Log/Log.h"
#include "Platform/FileSystem.h"

#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include <mono/metadata/appdomain.h>
#include <mono/metadata/object.h>
#include <mono/metadata/mono-gc.h>
#include <mono/metadata/reflection.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-debug.h>
#include <fmt/format.h>

namespace flaw {	
	MonoScriptClassField::MonoScriptClassField(MonoDomain* domain, MonoClass* parentClss, MonoClassField* fieldClass)
		: _domain(domain)
		, _parentClass(parentClss)
		, _field(fieldClass)
	{
		_monoType = mono_field_get_type(_field);
		_monoClass = mono_class_from_mono_type(_monoType);
	}

	bool MonoScriptClassField::IsPublic() const {
		return mono_field_get_flags(_field) & MONO_FIELD_ATTR_PUBLIC;
	}

	bool MonoScriptClassField::IsClass() const {
		return mono_type_get_type(_monoType) == MONO_TYPE_CLASS;
	}

	bool MonoScriptClassField::IsSubClassOf(MonoClass* baseClass) const {
		return mono_class_is_subclass_of(_monoClass, baseClass, false);
	}

	bool MonoScriptClassField::HasAttribute(MonoClass* attributeClass) const {
		MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(_parentClass, _field);
		if (!attrInfo) {
			return false;
		}

		bool hasAttr = mono_custom_attrs_has_attr(attrInfo, attributeClass);
		mono_custom_attrs_free(attrInfo);

		return hasAttr;
	}

	void MonoScriptClassField::SetValue(const MonoScriptObject* obj, void* value) {
		mono_field_set_value(obj->GetMonoObject(), _field, value);
	}

	void MonoScriptClassField::GetValue(const MonoScriptObject* obj, void* buff) const {
		mono_field_get_value(obj->GetMonoObject(), _field, buff);
	}

	std::string_view MonoScriptClassField::GetName() const {
		return mono_field_get_name(_field);
	}

	std::string_view MonoScriptClassField::GetTypeName() const {
		return mono_type_get_name(_monoType);
	}

	MonoScriptClass::MonoScriptClass(MonoDomain* appDomain, MonoClass* clss)
		: _domain(appDomain)
		, _clss(clss) 
	{
	}

	bool MonoScriptClass::IsSubClassOf(MonoScriptClass* baseClass) const {
		return mono_class_is_subclass_of(_clss, baseClass->GetMonoClass(), false);
	}

	bool MonoScriptClass::IsSubClassOf(MonoClass* baseClass) const {
		return mono_class_is_subclass_of(_clss, baseClass, false);
	}

	MonoScriptClassField MonoScriptClass::GetField(const char* fieldName) {
		MonoClassField* field = mono_class_get_field_from_name(_clss, fieldName);
		FASSERT(field, "Field not found");
		return MonoScriptClassField(_domain, _clss, field);
	}

	MonoScriptClassField MonoScriptClass::GetFieldRecursive(const char* fieldName) {
		MonoClass* clss = _clss;
		MonoClassField* field = nullptr;

		while (clss) {
			field = mono_class_get_field_from_name(clss, fieldName);
			if (field) {
				return MonoScriptClassField(_domain, clss, field);
			}
			clss = mono_class_get_parent(clss);
		}

		throw std::runtime_error(fmt::format("Field '{}' not found in class hierarchy", fieldName));
	}

	void MonoScriptClass::EachFields(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags) {
		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(_clss, &iter)) {
			const char* fieldName = mono_field_get_name(field);
			const uint32_t fieldFlags = mono_field_get_flags(field);

			if (fieldFlags & flags) {
				MonoScriptClassField fieldRef(_domain, _clss, field);
				callback(fieldName, fieldRef);
			}
		}
	}

	void MonoScriptClass::EachFieldsRecursive(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags) {
		MonoClass* clss = _clss;
		while (clss) {
			void* iter = nullptr;
			while (MonoClassField* field = mono_class_get_fields(clss, &iter)) {
				const char* fieldName = mono_field_get_name(field);
				const uint32_t fieldFlags = mono_field_get_flags(field);
				if (fieldFlags & flags) {
					MonoScriptClassField fieldRef(_domain, clss, field);
					callback(fieldName, fieldRef);
				}
			}
			clss = mono_class_get_parent(clss);
		}
	}

	MonoMethod* MonoScriptClass::GetMethodRecurcive(const char* methodName, int32_t argCount) {
		MonoClass* clss = _clss;
		MonoMethod* method = mono_class_get_method_from_name(clss, methodName, argCount);
		
		while (!method) {
			clss = mono_class_get_parent(clss);
			if (!clss) {
				break;
			}
			method = mono_class_get_method_from_name(clss, methodName, argCount);
		}

		return method;
	}

	MonoType* MonoScriptClass::GetMonoType() const {
		return mono_class_get_type(_clss);
	}

	MonoReflectionType* MonoScriptClass::GetReflectionType() const {
		return mono_type_get_object(mono_domain_get(), GetMonoType());
	}

	std::string_view MonoScriptClass::GetTypeName() const {
		return mono_class_get_name(_clss);
	}

	MonoScriptObject::MonoScriptObject(MonoDomain* domain, MonoClass* clss)
		: _domain(domain)
		, _clss(clss) 
		, _obj(nullptr)
	{
	}

	MonoScriptObject::MonoScriptObject(MonoDomain* domain, MonoClass* clss, MonoObject* obj)
		: _domain(domain)
		, _clss(clss)
		, _obj(obj)
	{
	}

	MonoScriptObject::MonoScriptObject(MonoScriptClass* clss)
		: _domain(clss->GetMonoDomain())
		, _clss(clss->GetMonoClass())
		, _obj(nullptr)
	{
	}

	MonoScriptObject::MonoScriptObject(MonoScriptClass* clss, MonoObject* obj)
		: _domain(clss->GetMonoDomain())
		, _clss(clss->GetMonoClass())
		, _obj(obj)
	{
	}

	void MonoScriptObject::Instantiate() {
		_obj = mono_object_new(_domain, _clss);
		if (!_obj) {
			throw std::runtime_error("mono_object_new failed");
		}

		mono_runtime_object_init(_obj);
	}

	void MonoScriptObject::CallMethod(MonoMethod* method, void** args, int32_t argCount) {
		MonoObject* exception = nullptr;
		MonoObject* result = mono_runtime_invoke(method, _obj, args, &exception);

		if (exception) {
			MonoString* excStr = mono_object_to_string(exception, nullptr);
			const char* excCStr = mono_string_to_utf8(excStr);
			Log::Error("Exception during method invocation: %s", excCStr);
			mono_free((void*)excCStr);
		}
	}

	static void DeepCloneFields(MonoScriptObject& source, MonoScriptObject& target, uint32_t fieldFlags) {
		source.GetClass().EachFieldsRecursive([&source, &target, fieldFlags](std::string_view fieldName, MonoScriptClassField& field) {
			auto targetField = target.GetClass().GetField(fieldName.data());
			if (!targetField) {
				return;
			}

			auto sourceFieldClass = field.GetMonoClass();
			auto targetFieldClass = targetField.GetMonoClass();
			if (sourceFieldClass != targetFieldClass) {
				return;
			}
			
			if (field.IsClass()) {
				MonoScriptObject sourceObj(source.GetMonoDomain(), sourceFieldClass, field.GetValue<MonoObject*>(&source));
				if (!sourceObj.IsValid()) {
					return;
				}

				MonoScriptObject targetObj(target.GetMonoDomain(), targetFieldClass);
				targetObj.Instantiate();
				targetField.SetValue(&target, targetObj.GetMonoObject());
				DeepCloneFields(sourceObj, targetObj, fieldFlags);
			}
			else {
				size_t size = mono_class_instance_size(field.GetMonoClass()) - sizeof(MonoObject);
				std::vector<int8_t> buff(size);
				field.GetValue(&source, buff.data());
				targetField.SetValue(&target, buff.data());
			}
		}, fieldFlags);
	}

	MonoScriptObject MonoScriptObject::Clone(uint32_t fieldFlags) {
		MonoScriptObject clone(_domain, _clss);
		clone.Instantiate();
		DeepCloneFields(*this, clone, fieldFlags);
		return clone;
	}

	void MonoScripting::Init() {
		// TODO: this should be configurable from outside
		mono_set_dirs("C:/Program Files/Mono/lib/", "C:/Program Files/Mono/etc/");

#if _DEBUG
		const char* argv[] = {
			"--debugger-agent=transport=dt_socket,address=127.0.0.1:55555,server=y,suspend=n,loglevel=3,logfile=MonoDebugger.log",
			"--soft-breakpoints"
		};

		mono_jit_parse_options(2, (char**)argv);
		mono_debug_init(MONO_DEBUG_FORMAT_MONO);
#endif

		auto rootDomain = mono_jit_init("FlawJITRuntime");
		if (!rootDomain) {
			throw std::runtime_error("mono_jit_init failed");
		}

#if _DEBUG
		mono_debug_domain_create(rootDomain);
#endif
	}

	void MonoScripting::Cleanup() {
		auto rootDomain = mono_get_root_domain();
		if (!rootDomain) {
			return;
		}

		mono_jit_cleanup(rootDomain);

#if _DEBUG
		mono_debug_cleanup();
#endif
	}

	void MonoScripting::RegisterInternalCall(const char* name, void* func) {
		mono_add_internal_call(name, func);
	}

	MonoScriptDomain::MonoScriptDomain() {
		char appDomainName[] = "FlawScriptRuntime";
		_appDomain = mono_domain_create_appdomain(appDomainName, nullptr);
		if (!_appDomain) {
			throw std::runtime_error("mono_domain_create_appdomain failed");
		}
	}

	MonoScriptDomain::~MonoScriptDomain() {
		if (mono_domain_get() == _appDomain) {
			mono_domain_set(mono_get_root_domain(), true);
		}

		mono_domain_unload(_appDomain);
	}

	void MonoScriptDomain::SetToCurrent() {
		mono_domain_set(_appDomain, true);
	}

	void MonoScriptDomain::AddMonoAssembly(const char* assemblyPath, bool loadPdb) {
		std::vector<int8_t> buffer;
		if (!FileSystem::ReadFile(assemblyPath, buffer)) {
			throw std::runtime_error("Failed to read assembly file");
		}

		MonoImageOpenStatus status;
		MonoImage* image = mono_image_open_from_data_full((char*)buffer.data(), buffer.size(), true, &status, false);
		if (status != MONO_IMAGE_OK) {
			const char* statusString = mono_image_strerror(status);
			throw std::runtime_error("mono_image_open failed: " + std::string(statusString));
		}

#if _DEBUG
		if (loadPdb) {
			std::filesystem::path pdbPath = assemblyPath;
			pdbPath.replace_extension(".pdb");

			if (!FileSystem::ReadFile(pdbPath.generic_string().c_str(), buffer)) {
				throw std::runtime_error("Failed to read pdb file");
			}

			mono_debug_open_image_from_memory(image, (const mono_byte*)buffer.data(), buffer.size());
		}
#endif

		MonoAssembly* assembly = mono_assembly_load_from_full(image, assemblyPath, &status, 0);
		mono_image_close(image);

		if (!assembly) {
			throw std::runtime_error("LoadMonoAssembly failed");
		}

		_assemblies.push_back(assembly);
	}

	void MonoScriptDomain::AddAllSubClassesOf(int32_t baseClassAssemblyIndex, const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex) {
		MonoClass* baseClass = mono_class_from_name(mono_assembly_get_image(_assemblies[baseClassAssemblyIndex]), baseClassNameSpace, baseClassName);
		if (!baseClass) {
			throw std::runtime_error("Base class not found");
		}

		MonoImage* image = mono_assembly_get_image(_assemblies[searchClassAssemblyIndex]);
		const MonoTableInfo* table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		uint32_t rows = mono_table_info_get_rows(table);

		for (int32_t i = 0; i < rows; i++) {
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(table, i, cols, MONO_TYPEDEF_SIZE);

			// Get namespace and type name
			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

			MonoClass* clss = mono_class_from_name(image, nameSpace, name);
			if (!clss) {
				continue;
			}

			if (mono_class_is_subclass_of(clss, baseClass, false)) {
				std::string fullName = fmt::format("{}.{}", nameSpace, name);
				_monoScriptClasses[fullName] = MonoScriptClass(_appDomain, clss);
			}
		}
	}

	void MonoScriptDomain::AddAllAttributesOf(int32_t assemblyIndex, const char* attributeNameSpace) {
		MonoImage* corlibImage = mono_get_corlib();
		MonoClass* attributeClass = mono_class_from_name(corlibImage, "System", "Attribute");

		MonoImage* image = mono_assembly_get_image(_assemblies[assemblyIndex]);
		const MonoTableInfo* table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		uint32_t rows = mono_table_info_get_rows(table);
		for (int32_t i = 0; i < rows; i++) {
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(table, i, cols, MONO_TYPEDEF_SIZE);

			// Get namespace and type name
			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);

			if (strcmp(nameSpace, attributeNameSpace) != 0) {
				continue; // Skip if namespace does not match
			}

			MonoClass* clss = mono_class_from_name(image, nameSpace, name);
			if (!clss) {
				continue;
			}

			if (mono_class_is_subclass_of(clss, attributeClass, false)) {
				std::string fullName = fmt::format("{}.{}", nameSpace, name);
				_monoScriptAttributes[fullName] = MonoScriptClass(_appDomain, clss);
			}
		}
	}

	void MonoScriptDomain::PrintMonoAssemblyInfo(int32_t assemblyIndex) {
		MonoImage* image = mono_assembly_get_image(_assemblies[assemblyIndex]);

		const MonoTableInfo* table = mono_image_get_table_info(image, MONO_TABLE_TYPEDEF);
		uint32_t rows = mono_table_info_get_rows(table);
		for (int32_t i = 0; i < rows; i++) {
			uint32_t cols[MONO_TYPEDEF_SIZE];
			mono_metadata_decode_row(table, i, cols, MONO_TYPEDEF_SIZE);
			// Get namespace and type name
			const char* nameSpace = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAMESPACE]);
			const char* name = mono_metadata_string_heap(image, cols[MONO_TYPEDEF_NAME]);
			if (nameSpace != nullptr && name != nullptr) {
				Log::Info("%s.%s", nameSpace, name);
			}
		}
	}

	bool MonoScriptDomain::IsClassExists(const char* name) {
		return _monoScriptClasses.find(name) != _monoScriptClasses.end();
	}

	MonoScriptObject MonoScriptDomain::CreateInstance(const char* name) {
		MonoScriptObject obj(&_monoScriptClasses[name]);
		obj.Instantiate();
		return obj;
	}

	MonoScriptObject MonoScriptDomain::CreateInstance(const char* name, void** constructorArgs, int32_t argCount) {
		auto it = _monoScriptClasses.find(name);
		if (it == _monoScriptClasses.end()) {
			Log::Error("Mono script class not found: %s", name);
			return nullptr;
		}

		auto scriptClass = it->second;
		auto constructorMethod = scriptClass.GetMethodRecurcive(".ctor", argCount);

		MonoScriptObject obj(&scriptClass);
		obj.Instantiate();
		obj.CallMethod(constructorMethod, constructorArgs, argCount);

		return obj;
	}

	MonoScriptClass& MonoScriptDomain::GetSystemClass(MonoSystemType type) const {
		MonoImage* corlibImage = mono_get_corlib();
		switch (type) {
			case MonoSystemType::Int32: {
				static MonoScriptClass int32Class(nullptr, nullptr);
				if (!int32Class) {
					MonoClass* clss = mono_class_from_name(corlibImage, "System", "Int32");
					int32Class = MonoScriptClass(_appDomain, clss);
				}
				return int32Class;
			}
			case MonoSystemType::Float: {
				static MonoScriptClass floatClass(nullptr, nullptr);
				if (!floatClass) {
					MonoClass* clss = mono_class_from_name(corlibImage, "System", "Single");
					floatClass = MonoScriptClass(_appDomain, clss);
				}
				return floatClass;
			}
		}

		throw std::runtime_error("Unknown MonoSystemType");
	}

	MonoScriptClass& MonoScriptDomain::GetClass(const char* name) {
		auto it = _monoScriptClasses.find(name);
		if (it != _monoScriptClasses.end()) {
			return it->second;
		}

		throw std::runtime_error(fmt::format("Mono script class not found: {}", name));
	}

	MonoScriptClass& MonoScriptDomain::GetAttribute(const char* name) {
		auto it = _monoScriptAttributes.find(name);
		if (it != _monoScriptAttributes.end()) {
			return it->second;
		}

		throw std::runtime_error(fmt::format("Mono script attribute not found: {}", name));
	}
}