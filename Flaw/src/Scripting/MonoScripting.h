#pragma once

#include "Core.h"

// forward declaration
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
}

namespace flaw {
	class MonoScriptDomain;
	class MonoScriptObject;
	class MonoScriptClass;

	class MonoScriptClassField {
	public:
		MonoScriptClassField() = default;
		MonoScriptClassField(MonoDomain* domain, MonoClass* parentClss, MonoClassField* fieldClass);

		bool IsPublic() const;
		bool IsClass() const;

		bool IsSubClassOf(MonoClass* baseClass) const;

		bool HasAttribute(MonoClass* attributeClass) const;

		void SetValue(const MonoScriptObject* obj, void* value);

		void GetValue(const MonoScriptObject* obj, void* buff) const;

		template <typename T>
		T GetValue(const MonoScriptObject* obj) {
			T value;
			GetValue(obj, &value);
			return value;
		}

		MonoClass* GetParentMonoClass() const { return _parentClass; }

		MonoClass* GetMonoClass() const { return _monoClass; }
		MonoClassField* GetMonoClassField() const { return _field; }

		MonoType* GetMonoType() const { return _monoType; }

		std::string_view GetName() const;
		std::string_view GetTypeName() const;

		operator bool() const { return _field != nullptr; }

	private:
		MonoDomain* _domain;
		MonoClass* _parentClass;
		MonoClass* _monoClass;
		MonoType* _monoType;
		MonoClassField* _field;
	};

	enum MonoFieldFlag {
		MonoFieldFlag_Private = 0x0001, // MONO_FIELD_ATTR_PRIVATE
		MonoFieldFlag_Protected = 0x0004, // MONO_FIELD_ATTR_FAMILY
		MonoFieldFlag_Public = 0x0006, // MONO_FIELD_ATTR_PUBLIC
		MonoFieldFlag_Static = 0x0010, // MONO_FIELD_ATTR_STATIC
		MonoFieldFlag_All = MonoFieldFlag_Private | MonoFieldFlag_Protected | MonoFieldFlag_Public | MonoFieldFlag_Static
	};

	class MonoScriptClass {
	public:
		MonoScriptClass() = default;
		MonoScriptClass(MonoDomain* appDomain, MonoClass* clss);

		bool IsSubClassOf(MonoScriptClass* baseClass) const;
		bool IsSubClassOf(MonoClass* baseClass) const;

		MonoScriptClassField GetField(const char* fieldName);
		MonoScriptClassField GetFieldRecursive(const char* fieldName);

		void EachFields(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags = MonoFieldFlag::MonoFieldFlag_Public);
		void EachFieldsRecursive(const std::function<void(std::string_view, MonoScriptClassField&)>& callback, uint32_t flags = MonoFieldFlag::MonoFieldFlag_Public);

		int32_t GetInstanceSize() const;

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

	struct MonoScriptObjectTreeNode {
		std::string name;
		std::string typeName;

		virtual ~MonoScriptObjectTreeNode() = default;
	};

	struct MonoScriptObjectTreeNodeObject : public MonoScriptObjectTreeNode {
		std::vector<Ref<MonoScriptObjectTreeNode>> elements;
	};

	struct MonoScriptObjectTreeNodeValue : public MonoScriptObjectTreeNode {
		std::vector<int8_t> valueData;
	};

	class MonoScriptObject {
	public:
		MonoScriptObject() = default;
		MonoScriptObject(MonoDomain* domain, MonoClass* clss);
		MonoScriptObject(MonoDomain* domain, MonoClass* clss, MonoObject* obj);
		MonoScriptObject(MonoScriptClass* clss);
		MonoScriptObject(MonoScriptClass* clss, MonoObject* obj);

		void Instantiate();

		void CallMethod(MonoMethod* method, void** args = nullptr, int32_t argCount = 0) const;

		MonoScriptObject Clone(uint32_t fieldFlags = MonoFieldFlag_Public) const;

		Ref<MonoScriptObjectTreeNode> ToTree() const;
		void ApplyTree(const Ref<MonoScriptObjectTreeNode>& treeNode);

		bool IsValid() const { return _obj != nullptr; }

		MonoScriptClass GetClass() const { return MonoScriptClass(_domain, _clss); }

		MonoDomain* GetMonoDomain() const { return _domain; }
		MonoClass* GetMonoClass() const { return _clss; }
		MonoObject* GetMonoObject() const { return _obj; }

	private:
		MonoDomain* _domain;
		MonoClass* _clss;
		MonoObject* _obj;
	};

	class MonoScripting {
	public:
		static void Init();
		static void Cleanup();

		static void RegisterInternalCall(const char* name, void* func);
	};

	enum class MonoSystemType {
		Int32,
		Float,
		Attribute,
	};

	class MonoScriptDomain {
	public:
		MonoScriptDomain();
		~MonoScriptDomain();

		void SetToCurrent();

		void AddMonoAssembly(const char* assemblyPath, bool loadPdb = false);

		void AddAllSubClassesOf(const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex);
		void AddAllSubClassesOf(int32_t baseClassAssemblyIndex, const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex);

		void PrintMonoAssemblyInfo(int32_t assemblyIndex);

		bool IsClassExists(const char* name);
		MonoScriptObject CreateInstance(const char* name);
		MonoScriptObject CreateInstance(const char* name, void** constructorArgs, int32_t argCount);

		MonoScriptClass& GetSystemClass(MonoSystemType type) const;
		MonoScriptClass& GetClass(const char* name);

		MonoDomain* GetMonoDomain() const { return _appDomain; }

		const std::unordered_map<std::string, MonoScriptClass>& GetMonoScriptClasses() const { return _monoScriptClasses; }

	private:
		void AddAllSubClassesOfImpl(MonoClass* baseClass, int32_t searchClassAssemblyIndex);

	private:
		MonoDomain* _appDomain;
		std::vector<MonoAssembly*> _assemblies;

		std::unordered_map<std::string, MonoScriptClass> _monoScriptClasses;
	};
}

