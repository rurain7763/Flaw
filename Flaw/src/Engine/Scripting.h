#pragma once

#include "Core.h"
#include "Scripting/MonoScripting.h"
#include "Utils/UUID.h"
#include "Entity.h"
#include "Asset.h"

namespace flaw {
	class Application;
	class Scene;
	class MonoScriptSystem;


	class Scripting {
	public:
		constexpr static const char* EntityClassName = "Flaw.Entity";

		static void Init(Application& app);
		static void Reload();
		static void Cleanup();

		static void SetActiveMonoScriptSystem(MonoScriptSystem* system);

		static MonoScriptClass& GetMonoSystemClass(MonoSystemType type);
		static MonoScriptClass& GetMonoAssetClass(AssetType type);
		static MonoScriptClass& GetMonoClass(const char* name);

		static bool HasMonoClass(const char* name);

		static bool IsMonoComponent(const MonoScriptClass& monoClass);
		static bool IsMonoProjectComponent(const MonoScriptClass& monoClass);
		static bool IsMonoAsset(const MonoScriptClass& monoClass);

		static bool HasEngineComponent(const Entity& entity, const char* compName);

		static MonoScriptDomain& GetMonoScriptDomain();
		static Application& GetApplication();
		static Scene& GetScene();

	private:
		static void LoadMonoScripting();

		static MonoObject* GetEntity(UUID uuid);
		static bool HasComponent(UUID uuid, MonoReflectionType* type);
		static MonoObject* GetComponentInstance(UUID uuid, MonoReflectionType* type);
		static float GetDeltaTime();
		static float GetTimeSinceStart();
	};
}