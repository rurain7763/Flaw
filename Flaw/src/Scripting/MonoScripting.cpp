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
	static MonoDomain* s_rootDomain = nullptr;
	
	MonoScriptClassField::MonoScriptClassField(MonoClassField* field)
		: _field(field)
	{
	}

	bool MonoScriptClassField::IsPublic() const {
		return mono_field_get_flags(_field) & MONO_FIELD_ATTR_PUBLIC;
	}

	bool MonoScriptClassField::IsClass() const {
		MonoType* type = mono_field_get_type(_field);
		return mono_type_get_type(type) == MONO_TYPE_CLASS;
	}

	void MonoScriptClassField::SetValue(MonoScriptObject* obj, void* value) {
		mono_field_set_value(obj->GetMonoObject(), _field, value);
	}

	void MonoScriptClassField::GetValueImpl(MonoScriptObject* obj, void* buff) {
		mono_field_get_value(obj->GetMonoObject(), _field, buff);
	}

	MonoType* MonoScriptClassField::GetMonoType() const {
		return mono_field_get_type(_field);
	}

	std::string_view MonoScriptClassField::GetName() const {
		return mono_field_get_name(_field);
	}

	std::string_view MonoScriptClassField::GetTypeName() const {
		MonoType* type = mono_field_get_type(_field);
		return mono_type_get_name(type);
	}

	MonoScriptClass::MonoScriptClass(MonoDomain* appDomain, MonoClass* clss) 
		: _appDomain(appDomain)
		, _clss(clss) 
	{
	}

	MonoObject* MonoScriptClass::CreateInstance() {
		MonoObject* obj = mono_object_new(_appDomain, _clss);
		if (!obj) {
			throw std::runtime_error("mono_object_new failed");
		}

		mono_runtime_object_init(obj);
		return obj;
	}

	MonoScriptClassField MonoScriptClass::GetField(const char* fieldName) {
		MonoClassField* field = mono_class_get_field_from_name(_clss, fieldName);
		return MonoScriptClassField(field);
	}

	void MonoScriptClass::EachPublicFields(std::function<void(std::string_view, MonoScriptClassField&)> callback) {
		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(_clss, &iter)) {
			const char* fieldName = mono_field_get_name(field);
			int32_t flags = mono_field_get_flags(field);

			MonoScriptClassField fieldRef(field);
			if (fieldRef.IsPublic()) {
				callback(fieldName, fieldRef);
			}
		}
	}

	MonoMethod* MonoScriptClass::GetMethod(const char* methodName, int32_t argCount) {
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

	std::string_view MonoScriptClass::GetTypeName() const {
		return mono_class_get_name(_clss);
	}

	MonoScriptObject::MonoScriptObject(MonoScriptClass* clss)
		: _clss(clss) 
		, _obj(nullptr)
	{
	}

	void MonoScriptObject::Instantiate() {
		_obj = _clss->CreateInstance();
	}

	void MonoScriptObject::SaveMethod(const char* methodName, int32_t argCount, int32_t slot) {
		if (slot >= _savedMethods.size()) {
			throw std::runtime_error("slot not valid, we only support 0 - 1 slots");
		}

		MonoMethod* method = _clss->GetMethod(methodName, argCount);
		if (!method) {
			throw std::runtime_error("GetMethod failed");
		}

		_savedMethods[slot] = method;
	}

	void MonoScriptObject::CallMethod(int32_t methodIndex, void** args, int32_t argCount) {
		mono_runtime_invoke(_savedMethods[methodIndex], _obj, args, nullptr);
	}

	void MonoScriptObject::CallMethod(const char* methodName, void** args, int32_t argCount) {
		MonoMethod* method = _clss->GetMethod(methodName, argCount);
		if (!method) {
			Log::Warn("Method never be called, because it's not found: %s", methodName);
			return;
		}

		MonoObject* exception = nullptr;
		MonoObject* result = mono_runtime_invoke(method, _obj, args, &exception);

		if (exception) {
			MonoString* excStr = mono_object_to_string(exception, nullptr);
			const char* excCStr = mono_string_to_utf8(excStr);
			Log::Error("Exception during method invocation: %s", excCStr);
			mono_free((void*)excCStr);
		}
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

		s_rootDomain = mono_jit_init("FlawJITRuntime");
		if (!s_rootDomain) {
			throw std::runtime_error("mono_jit_init failed");
		}

#if _DEBUG
		mono_debug_domain_create(s_rootDomain);
#endif
	}

	void MonoScripting::Cleanup() {
		if (!s_rootDomain) {
			return;
		}

		mono_jit_cleanup(s_rootDomain);
		s_rootDomain = nullptr;

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
			mono_domain_set(s_rootDomain, true);
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

	void MonoScriptDomain::AddAllSubClassesOf(
		int32_t baseClassAssemblyIndex,
		const char* baseClassNameSpace,
		const char* baseClassName,
		int32_t searchClassAssemblyIndex)
	{
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

			if (baseClass != clss && mono_class_is_subclass_of(clss, baseClass, false)) {
				std::string fullName = fmt::format("{}.{}", nameSpace, name);
				_monoScriptClasses[fullName] = CreateRef<MonoScriptClass>(_appDomain, clss);
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

	Ref<MonoScriptObject> MonoScriptDomain::CreateInstance(const char* name) {
		auto obj = CreateRef<MonoScriptObject>(_monoScriptClasses[name].get());
		obj->Instantiate();
		return obj;
	}

	MonoType* MonoScriptDomain::GetReflectionType(const char* name) {
		for (auto assem : _assemblies) {
			MonoType* type = mono_reflection_type_from_name((char*)name, mono_assembly_get_image(assem));
			if (type) {
				return type;
			}
		}
		return nullptr;
	}
}