#pragma once

extern "C" {
	typedef struct _MonoObject MonoObject;
	typedef struct _MonoAssembly MonoAssembly;
	typedef struct _MonoDomain MonoDomain;
	typedef struct _MonoImage MonoImage;
	typedef struct _MonoClass MonoClass;
	typedef struct _MonoMethod MonoMethod;
	typedef struct _MonoString MonoString;
	typedef struct _MonoTableInfo MonoTableInfo;
	typedef struct _MonoType MonoType;
	typedef struct _MonoClassField MonoClassField;
	typedef struct _MonoReflectionType MonoReflectionType;
	typedef struct _MonoArray MonoArray;
}

namespace flaw {
	enum MonoFieldFlag {
		MonoFieldFlag_Private = 0x0001, // MONO_FIELD_ATTR_PRIVATE
		MonoFieldFlag_Protected = 0x0004, // MONO_FIELD_ATTR_FAMILY
		MonoFieldFlag_Public = 0x0006, // MONO_FIELD_ATTR_PUBLIC
		MonoFieldFlag_Static = 0x0010, // MONO_FIELD_ATTR_STATIC
		MonoFieldFlag_All = MonoFieldFlag_Private | MonoFieldFlag_Protected | MonoFieldFlag_Public | MonoFieldFlag_Static
	};
}