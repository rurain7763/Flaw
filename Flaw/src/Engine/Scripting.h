#pragma once

#include "Core.h"
#include "Scripting/MonoScripting.h"
#include "Utils/UUID.h"

namespace flaw {
	class Application;
	class Scene;

	class Scripting {
	public:
		static void Init(Application& app);
		static void Reload();
		static void Cleanup();

		static void OnStart(Scene* scene);
		static void OnUpdate();
		static void OnEnd();

		static Ref<MonoScriptObject> CreateTempMonoScriptObject(const UUID& uuid, const char* name);
		static void DestroyTempMonoScriptObject(const UUID& uuid);
		static Ref<MonoScriptObject> GetTempMonoScriptObject(const UUID& uuid);

		static bool IsEngineComponent(MonoReflectionType* type);
		static bool HasComponent(UUID uuid, MonoReflectionType* type);
		static bool IsComponentInstanceExists(UUID uuid);
		static MonoObject* GetComponentInstance(UUID uuid);

		static MonoScriptDomain& GetMonoScriptDomain();
		static Application& GetApplication();
		static Scene& GetScene();
		static float GetTimeSinceStart();

	private:
		static Ref<MonoScriptObject> CreateMonoScriptObjectImpl(const UUID& uuid, const char* name);

	private:
		static void LoadMonoScripts();
	};
}