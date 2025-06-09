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
		MonoScriptClassField(MonoScriptClass* clss, MonoClassField* field);

		bool IsPublic() const;
		bool IsClass() const;

		bool HasAttribute(const char* attributeName) const;

		void SetValue(MonoScriptObject* obj, void* value);

		void GetValue(MonoScriptObject* obj, void* buff);

		template <typename T>
		T GetValue(MonoScriptObject* obj) {
			T value;
			GetValue(obj, &value);
			return value;
		}

		MonoClassField* GetMonoClassField() const { return _field; }

		MonoType* GetMonoType() const;

		std::string_view GetName() const;
		std::string_view GetTypeName() const;

		operator bool() const { return _field != nullptr; }

	private:
		MonoScriptClass* _clss;
		MonoClassField* _field;
	};

	class MonoScriptClass {
	public:
		MonoScriptClass(MonoScriptDomain* appDomain, MonoClass* clss);

		MonoObject* CreateInstance();

		MonoScriptClassField GetField(const char* fieldName);
		void EachPublicFields(std::function<void(std::string_view, MonoScriptClassField&)> callback);

		MonoMethod* GetMethod(const char* methodName, int32_t argCount);

		MonoType* GetMonoType() const;
		MonoReflectionType* GetReflectionType() const;

		std::string_view GetTypeName() const;

		MonoScriptDomain* GetDomain() const { return _domain; }
		MonoClass* GetMonoClass() const { return _clss; }

	private:
		MonoScriptDomain* _domain;
		MonoClass* _clss;
	};

	class MonoScriptObject {
	public:
		MonoScriptObject(MonoScriptClass* clss);

		void Instantiate();

		void SaveMethod(const char* methodName, int32_t argCount, int32_t slot);

		void CallMethod(int32_t methodIndex, void** args = nullptr, int32_t argCount = 0);
		void CallMethod(const char* methodName, void** args = nullptr, int32_t argCount = 0);

		MonoScriptClass* GetClass() const { return _clss; }

		MonoObject* GetMonoObject() const { return _obj; }

	private:
		MonoScriptClass* _clss;
		MonoObject* _obj;

		std::array<MonoMethod*, 2> _savedMethods;
	};

	class MonoScripting {
	public:
		static void Init();
		static void Cleanup();

		static void RegisterInternalCall(const char* name, void* func);
	};

	class MonoScriptDomain {
	public:
		MonoScriptDomain();
		~MonoScriptDomain();

		void SetToCurrent();

		void AddMonoAssembly(const char* assemblyPath, bool loadPdb = false);

		void AddAllSubClassesOf(
			int32_t baseClassAssemblyIndex, 
			const char* baseClassNameSpace, 
			const char* baseClassName, 
			int32_t searchClassAssemblyIndex
		);

		void AddAllAttributesOf(int32_t assemblyIndex, const char* attributeNameSpace);
		
		void PrintMonoAssemblyInfo(int32_t assemblyIndex);

		bool IsClassExists(const char* name);
		Ref<MonoScriptObject> CreateInstance(const char* name);

		Ref<MonoScriptClass> GetClass(const char* name);
		Ref<MonoScriptClass> GetAttribute(const char* name);

		MonoDomain* GetMonoDomain() const { return _appDomain; }
		const std::unordered_map<std::string, Ref<MonoScriptClass>>& GetMonoScriptClasses() const { return _monoScriptClasses; }
		const std::unordered_map<std::string, Ref<MonoScriptClass>>& GetMonoScriptAttributes() const { return _monoScriptAttributes; }

	private:
		MonoDomain* _appDomain;
		std::vector<MonoAssembly*> _assemblies;

		std::unordered_map<std::string, Ref<MonoScriptClass>> _monoScriptClasses;
		std::unordered_map<std::string, Ref<MonoScriptClass>> _monoScriptAttributes;
	};
}

