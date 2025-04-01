#pragma once

#include "Core.h"
#include "Scripting/MonoScripting.h"
#include "Entity.h"

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

		static Ref<MonoScriptObject> CreateMonoScriptObject(const Entity& entity, const char* name);
		static Ref<MonoScriptObject> GetMonoScriptObject(const Entity& entity);

	private:
		static void LoadMonoScripts();
	};
}