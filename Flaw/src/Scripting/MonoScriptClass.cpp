#include "pch.h"
#include "MonoScriptClass.h"
#include "MonoScriptClassField.h"

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
	MonoScriptClass::MonoScriptClass()
		: _domain(nullptr)
		, _monoClass(nullptr)
		, _monoType(nullptr)
		, _reflectionType(nullptr)
	{
	}

	MonoScriptClass::MonoScriptClass(MonoDomain* domain, MonoClass* clss)
		: _domain(domain)
		, _monoClass(clss)
		, _monoType(nullptr)
		, _reflectionType(nullptr)
	{
		if (_monoClass) {
			_monoType = mono_class_get_type(_monoClass);
			_reflectionType = mono_type_get_object(domain, _monoType);
		}
	}

	bool MonoScriptClass::IsSubClassOf(MonoScriptClass& baseClass) const {
		return mono_class_is_subclass_of(_monoClass, baseClass.GetMonoClass(), false);
	}

	MonoScriptClassField MonoScriptClass::GetField(const char* fieldName) {
		MonoClassField* field = mono_class_get_field_from_name(_monoClass, fieldName);
		return MonoScriptClassField(_domain, field);
	}

	MonoScriptClassField MonoScriptClass::GetFieldRecursive(const char* fieldName) {
		MonoClass* clss = _monoClass;
		MonoClassField* field = nullptr;

		while (clss) {
			field = mono_class_get_field_from_name(clss, fieldName);
			if (field) {
				return MonoScriptClassField(_domain, field);
			}
			clss = mono_class_get_parent(clss);
		}

		return MonoScriptClassField(_domain, nullptr);
	}

	void MonoScriptClass::EachFields(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags) {
		void* iter = nullptr;
		while (MonoClassField* field = mono_class_get_fields(_monoClass, &iter)) {
			const char* fieldName = mono_field_get_name(field);
			const uint32_t fieldFlags = mono_field_get_flags(field);

			if (fieldFlags & flags) {
				MonoScriptClassField fieldRef(_domain, field);
				callback(fieldName, fieldRef);
			}
		}
	}

	void MonoScriptClass::EachFieldsRecursive(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags) {
		MonoClass* clss = _monoClass;
		while (clss) {
			void* iter = nullptr;
			while (MonoClassField* field = mono_class_get_fields(clss, &iter)) {
				const char* fieldName = mono_field_get_name(field);
				const uint32_t fieldFlags = mono_field_get_flags(field);
				if (fieldFlags & flags) {
					MonoScriptClassField fieldRef(_domain, field);
					callback(fieldName, fieldRef);
				}
			}
			clss = mono_class_get_parent(clss);
		}
	}

	int32_t MonoScriptClass::GetTypeSize() const {
		return mono_type_size(_monoType, nullptr);
	}

	MonoMethod* MonoScriptClass::GetMethodRecurcive(const char* methodName, int32_t argCount) {
		MonoClass* clss = _monoClass;
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
		return mono_type_get_name_full(_monoType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
	}
}