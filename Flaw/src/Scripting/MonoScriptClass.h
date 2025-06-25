#pragma once

#include "Core.h"
#include "MonoScriptTypes.h"

namespace flaw {
	class MonoScriptClassField;

	class MonoScriptClass {
	public:
		MonoScriptClass();
		MonoScriptClass(MonoDomain* domain, MonoClass* clss);

		bool IsSubClassOf(MonoScriptClass& baseClass) const;

		MonoScriptClassField GetField(const char* fieldName);
		MonoScriptClassField GetFieldRecursive(const char* fieldName);

		void EachFields(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags = MonoFieldFlag::MonoFieldFlag_Public);
		void EachFieldsRecursive(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags = MonoFieldFlag::MonoFieldFlag_Public);

		int32_t GetTypeSize() const;

		MonoMethod* GetMethodRecurcive(const char* methodName, int32_t argCount);

		std::string_view GetTypeName() const;

		MonoType* GetMonoType() const { return _monoType; }
		MonoReflectionType* GetReflectionType() const { return _reflectionType; }
		MonoDomain* GetMonoDomain() const { return _domain; }
		MonoClass* GetMonoClass() const { return _monoClass; }

		explicit operator bool() const { return _monoClass != nullptr; }

		bool operator==(const MonoScriptClass& other) const {
			return _monoClass == other._monoClass;
		}

		bool operator!=(const MonoScriptClass& other) const {
			return !(*this == other);
		}

	private:
		MonoDomain* _domain;
		MonoClass* _monoClass;
		MonoType* _monoType;
		MonoReflectionType* _reflectionType;
	};
}