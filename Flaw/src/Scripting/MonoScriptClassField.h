#pragma once

#include "Core.h"
#include "MonoScriptTypes.h"

namespace flaw {
	class MonoScriptClass;
	class MonoScriptObject;
	class MonoScriptObjectView;

	class MonoScriptClassField {
	public:
		MonoScriptClassField() = default;
		MonoScriptClassField(MonoDomain* domain, MonoClassField* fieldClass);

		bool IsPublic() const;
		bool IsClass() const;

		bool HasAttribute(const MonoScriptClass& attributeClass) const;

		void SetValue(const MonoScriptObject& obj, void* value);
		void SetValue(const MonoScriptObjectView& objView, void* value);

		void GetValue(const MonoScriptObject& obj, void* buff) const;
		void GetValue(const MonoScriptObjectView& objView, void* buff) const;

		template <typename T>
		T GetValue(const MonoScriptObject& obj) {
			T value;
			GetValue(obj, &value);
			return value;
		}

		template <typename T>
		T GetValue(const MonoScriptObjectView& objView) {
			T value;
			GetValue(objView, &value);
			return value;
		}

		MonoScriptClass GetClass() const;

		MonoDomain* GetMonoDomain() const { return _domain; }
		MonoClassField* GetMonoClassField() const { return _field; }

		std::string_view GetName() const;

		operator bool() const { return _field != nullptr; }

	private:
		MonoDomain* _domain;
		MonoClassField* _field;
	};
}