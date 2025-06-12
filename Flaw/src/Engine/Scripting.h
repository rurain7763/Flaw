#pragma once

#include "Core.h"
#include "Scripting/MonoScripting.h"
#include "Utils/UUID.h"

namespace flaw {
	class Application;
	class Scene;
	class MonoScriptSystem;

	enum class MonoAssetType {
		Prefab,
	};

	class Scripting {
	public:
		static void Init(Application& app);
		static void Reload();
		static void Cleanup();

		static void RegisterMonoScriptSystem(MonoScriptSystem* system);
		static void UnregisterMonoScriptSystem(MonoScriptSystem* system);
		static void SetActiveMonoScriptSystem(MonoScriptSystem* system);

		static MonoScriptClass& GetMonoSystemClass(MonoSystemType type);
		static MonoScriptClass& GetMonoAssetClass(MonoAssetType type);
		static MonoScriptClass& GetMonoClass(const char* name);

		static bool IsMonoComponent(const MonoScriptClass& monoClass);
		static bool IsEngineComponent(MonoReflectionType* type);
		static bool HasComponent(UUID uuid, MonoReflectionType* type);
		static bool IsComponentInstanceExists(UUID uuid);
		static MonoObject* GetComponentInstance(UUID uuid);
		static float GetDeltaTime();
		static float GetTimeSinceStart();

		static MonoScriptDomain& GetMonoScriptDomain();
		static Application& GetApplication();
		static Scene& GetScene();

	private:
		static void LoadMonoScripts();
	};
}