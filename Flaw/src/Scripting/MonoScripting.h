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
}

namespace flaw {
	class MonoScriptObject;

	class MonoScriptClassField {
	public:
		MonoScriptClassField(MonoClassField* field);

		bool IsPublic() const;
		bool IsClass() const;

		void SetValue(MonoScriptObject* obj, void* value);

		template <typename T>
		T GetValue(MonoScriptObject* obj) {
			T value;
			GetValueImpl(obj, &value);
			return value;
		}

		std::string_view GetTypeName() const;

	private:
		void GetValueImpl(MonoScriptObject* obj, void* buff);

	private:
		MonoClassField* _field;
	};

	class MonoScriptClass {
	public:
		MonoScriptClass(MonoDomain* appDomain, MonoClass* clss);

		MonoObject* CreateInstance();

		void EachFields(std::function<void(std::string_view, MonoScriptClassField&)> callback);

		MonoMethod* GetMethod(const char* methodName, int32_t argCount);

		MonoClass* GetNativeClass() const { return _clss; }

	private:
		MonoDomain* _appDomain;
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

		MonoObject* GetNativeObject() const { return _obj; }

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
		
		void PrintMonoAssemblyInfo(int32_t assemblyIndex);

		bool IsClassExists(const char* name);
		Ref<MonoScriptObject> CreateInstance(const char* name);

		MonoType* GetReflectionType(const char* name);

	private:
		MonoDomain* _appDomain;
		std::vector<MonoAssembly*> _assemblies;

		std::unordered_map<std::string, Ref<MonoScriptClass>> _monoScriptClasses;
	};
}

