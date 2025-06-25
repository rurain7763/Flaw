#include "pch.h"
#include "MonoScriptClassField.h"
#include "MonoScriptObject.h"
#include "MonoScriptClass.h"

#include <mono/metadata/object.h>
#include <mono/metadata/attrdefs.h>
#include <mono/metadata/mono-debug.h>

namespace flaw {
	MonoScriptClassField::MonoScriptClassField(MonoDomain* domain, MonoClassField* fieldClass)
		: _domain(domain)
		, _field(fieldClass)
	{
	}

	bool MonoScriptClassField::IsPublic() const {
		return mono_field_get_flags(_field) & MONO_FIELD_ATTR_PUBLIC;
	}

	bool MonoScriptClassField::IsClass() const {
		MonoType* monoType = mono_field_get_type(_field);
		return mono_type_get_type(monoType) == MONO_TYPE_CLASS;
	}

	bool MonoScriptClassField::HasAttribute(const MonoScriptClass& attributeClass) const {
		MonoClass* parentClass = mono_field_get_parent(_field);
		MonoCustomAttrInfo* attrInfo = mono_custom_attrs_from_field(parentClass, _field);
		if (!attrInfo) {
			return false;
		}

		bool hasAttr = mono_custom_attrs_has_attr(attrInfo, attributeClass.GetMonoClass());
		mono_custom_attrs_free(attrInfo);

		return hasAttr;
	}

	void MonoScriptClassField::SetValue(const MonoScriptObject& obj, void* value) {
		mono_field_set_value(obj.GetMonoObject(), _field, value);
	}

	void MonoScriptClassField::SetValue(const MonoScriptObjectView& objView, void* value) {
		mono_field_set_value(objView.GetMonoObject(), _field, value);
	}

	void MonoScriptClassField::GetValue(const MonoScriptObject& obj, void* buff) const {
		mono_field_get_value(obj.GetMonoObject(), _field, buff);
	}

	void MonoScriptClassField::GetValue(const MonoScriptObjectView& objView, void* buff) const {
		mono_field_get_value(objView.GetMonoObject(), _field, buff);
	}

	MonoScriptClass MonoScriptClassField::GetClass() const {
		MonoType* monoType = mono_field_get_type(_field);
		MonoClass* monoClass = mono_class_from_mono_type(monoType);

		return MonoScriptClass(_domain, monoClass);
	}

	std::string_view MonoScriptClassField::GetName() const {
		return mono_field_get_name(_field);
	}
}