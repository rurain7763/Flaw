#include "pch.h"
#include "MonoScriptDomain.h"
#include "MonoScriptClass.h"
#include "Platform/FileSystem.h"
#include "Log/Log.h"

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

	void MonoScriptDomain::AddClass(const char* nameSpace, const char* name, int32_t searchClassAssemblyIndex) {
		MonoImage* image = mono_assembly_get_image(_assemblies[searchClassAssemblyIndex]);
		MonoClass* clss = mono_class_from_name(image, nameSpace, name);
		if (!clss) {
			throw std::runtime_error(fmt::format("Class {}.{} not found", nameSpace, name));
		}
		std::string fullName = mono_type_get_name_full(mono_class_get_type(clss), MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
		_monoScriptClasses[fullName] = MonoScriptClass(_appDomain, clss);
	}

	void MonoScriptDomain::AddAllSubClassesOfImpl(MonoClass* baseClass, int32_t searchClassAssemblyIndex) {
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
				std::string fullName = mono_type_get_name_full(mono_class_get_type(clss), MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
				_monoScriptClasses[fullName] = MonoScriptClass(_appDomain, clss);
			}
		}
	}

	void MonoScriptDomain::AddAllSubClassesOf(const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex) {
		MonoImage* corlibImage = mono_get_corlib();
		MonoClass* baseClass = mono_class_from_name(corlibImage, baseClassNameSpace, baseClassName);
		if (!baseClass) {
			throw std::runtime_error("Base class not found");
		}

		AddAllSubClassesOfImpl(baseClass, searchClassAssemblyIndex);
	}

	void MonoScriptDomain::AddAllSubClassesOf(int32_t baseClassAssemblyIndex, const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex) {
		MonoClass* baseClass = mono_class_from_name(mono_assembly_get_image(_assemblies[baseClassAssemblyIndex]), baseClassNameSpace, baseClassName);
		if (!baseClass) {
			throw std::runtime_error("Base class not found");
		}

		AddAllSubClassesOfImpl(baseClass, searchClassAssemblyIndex);
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

			if (std::strcmp(name, "<Module>") == 0) {
				continue; // Skip module class
			}

			MonoClass* clss = mono_class_from_name(image, nameSpace, name);
			if (clss) {
				std::string fullName = mono_type_get_name_full(mono_class_get_type(clss), MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
				Log::Info("Mono class: %s", fullName.c_str());
			}
		}
	}

	bool MonoScriptDomain::IsClassExists(const char* name) {
		return _monoScriptClasses.find(name) != _monoScriptClasses.end();
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
		case MonoSystemType::Attribute: {
			static MonoScriptClass attributeClass(nullptr, nullptr);
			if (!attributeClass) {
				MonoClass* clss = mono_class_from_name(corlibImage, "System", "Attribute");
				attributeClass = MonoScriptClass(_appDomain, clss);
			}
			return attributeClass;
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
}