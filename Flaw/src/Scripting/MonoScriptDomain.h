#pragma once

#include "Core.h"
#include "MonoScriptTypes.h"

#include <unordered_map>

namespace flaw {
	class MonoScriptClass;

	enum class MonoSystemType {
		Int32,
		Float,
		Attribute,
	};

	class MonoScriptDomain {
	public:
		MonoScriptDomain();
		~MonoScriptDomain();

		void AddMonoAssembly(const char* assemblyPath, bool loadPdb = false);

		void AddClass(const char* nameSpace, const char* name, int32_t searchClassAssemblyIndex);
		void AddAllSubClassesOf(const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex);
		void AddAllSubClassesOf(int32_t baseClassAssemblyIndex, const char* baseClassNameSpace, const char* baseClassName, int32_t searchClassAssemblyIndex);

		void PrintMonoAssemblyInfo(int32_t assemblyIndex);

		bool IsClassExists(const char* name);

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
