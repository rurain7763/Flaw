#include "pch.h"
#include "Scripting.h"
#include "Log/Log.h"
#include "Math/Math.h"
#include "Components.h"
#include "Scene.h"
#include "Input/Input.h"
#include "Time/Time.h"
#include "Platform/FileWatch.h"
#include "Application.h"
#include "Project.h"

#include <mono/jit/jit.h>
#include <mono/metadata/reflection.h>
#include <fmt/format.h>

namespace flaw {
	#define ADD_INTERNAL_CALL(func) MonoScripting::RegisterInternalCall("Flaw.InternalCalls::"#func, func)

	static Scope<filewatch::FileWatch<std::string>> s_scriptAsmWatcher;

	static Scope<MonoScriptDomain> s_monoScriptDomain;
	static std::unordered_map<MonoType*, std::function<bool(const Entity&)>> s_hasComponentFuncs;

	static Application* s_app;
	static Scene* s_scene;
	static std::unordered_map<entt::entity, Ref<MonoScriptObject>> s_monoScriptObjects;

	// ======================== <internal calls> ========================
	static void LogInfo(MonoString* text) {
		char* t = mono_string_to_utf8(text);
		Log::Info("%s", t);
		mono_free(t);
	}

	static float GetDeltaTime() {
		return Time::DeltaTime();
	}

	static bool IsEngineComponent(MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);
		return s_hasComponentFuncs.find(monoType) != s_hasComponentFuncs.end();
	}

	static bool HasComponent(entt::entity entity, MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);
		FASSERT(s_hasComponentFuncs.find(monoType) != s_hasComponentFuncs.end(), "Component type not found");
		return s_hasComponentFuncs.at(monoType)(Entity(entity, s_scene));
	}

	static bool IsComponentInstanceExists(entt::entity entity) {
		return s_monoScriptObjects.find(entity) != s_monoScriptObjects.end();
	}

	static MonoObject* GetComponentInstance(entt::entity entity) {
		FASSERT(IsComponentInstanceExists(entity), "Component instance not found");
		return s_monoScriptObjects[entity]->GetMonoObject();
	}

	static uint32_t FindEntityByName(MonoString* name) {
		uint32_t found = std::numeric_limits<uint32_t>::max();

		char* n = mono_string_to_utf8(name);
		Entity entity = s_scene->FindEntityByName(n);
		if (entity) {
			found = (uint32_t)entity;
		}
		mono_free(n);

		return found;
	}

	static void GetPosition_Transform(entt::entity entity, vec3& position) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			position = comp.position;
		}
	}

	static void SetPosition_Transform(entt::entity entity, vec3& position) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			comp.position = position;
		}
	}

	static void GetRotation_Transform(entt::entity entity, vec3& rotation) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			rotation = comp.rotation;
		}
	}

	static void SetRotation_Transform(entt::entity entity, vec3& rotation) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			comp.rotation = rotation;
		}
	}

	static void GetScale_Transform(entt::entity entity, vec3& scale) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			scale = comp.scale;
		}
	}

	static void SetScale_Transform(entt::entity entity, vec3& scale) {
		Entity e(entity, s_scene);
		if (e.HasComponent<TransformComponent>()) {
			auto& comp = e.GetComponent<TransformComponent>();
			comp.scale = scale;
		}
	}

	static void GetBodyType_RigidBody2D(entt::entity entity, int32_t& bodyType) {
		Entity e(entity, s_scene);
		if (e.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = e.GetComponent<Rigidbody2DComponent>();
			bodyType = (int32_t)comp.bodyType;
		}
	}

	static void SetBodyType_RigidBody2D(entt::entity entity, int32_t bodyType) {
		Entity e(entity, s_scene);
		if (e.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = e.GetComponent<Rigidbody2DComponent>();
			comp.bodyType = (Rigidbody2DComponent::BodyType)bodyType;
		}
	}

	static void GetLinearVelocity_RigidBody2D(entt::entity entity, vec2& velocity) {
		Entity e(entity, s_scene);
		if (e.HasComponent<Rigidbody2DComponent>()) {
			auto& comp = e.GetComponent<Rigidbody2DComponent>();
			velocity = comp.linearVelocity;
		}
	}

	static bool GetKeyDown(KeyCode key) {
		return Input::GetKeyDown(key);
	}

	static bool GetKeyUp(KeyCode key) {
		return Input::GetKeyUp(key);
	}

	static bool GetKey(KeyCode key) {
		return Input::GetKey(key);
	}
	// ======================== </internal calls> ========================

	template <typename T>
	static void RegisterComponent() {
		const std::string fullName = fmt::format("Flaw.{}", TypeName<T>());
		MonoType* type = s_monoScriptDomain->GetReflectionType(fullName.c_str());
		if (type) {
			s_hasComponentFuncs[type] = [](const Entity& entity) { return entity.HasComponent<T>(); };
		}
	}

	void Scripting::Init(Application& app) {
		s_app = &app;

		MonoScripting::Init();

		// Register internal calls
		ADD_INTERNAL_CALL(LogInfo);
		ADD_INTERNAL_CALL(GetDeltaTime);
		ADD_INTERNAL_CALL(IsEngineComponent);
		ADD_INTERNAL_CALL(HasComponent);
		ADD_INTERNAL_CALL(IsComponentInstanceExists);
		ADD_INTERNAL_CALL(GetComponentInstance);
		ADD_INTERNAL_CALL(FindEntityByName);
		ADD_INTERNAL_CALL(GetPosition_Transform);
		ADD_INTERNAL_CALL(SetPosition_Transform);
		ADD_INTERNAL_CALL(GetRotation_Transform);
		ADD_INTERNAL_CALL(SetRotation_Transform);
		ADD_INTERNAL_CALL(GetScale_Transform);
		ADD_INTERNAL_CALL(SetScale_Transform);
		ADD_INTERNAL_CALL(GetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(SetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(GetLinearVelocity_RigidBody2D);
		ADD_INTERNAL_CALL(GetKeyDown);
		ADD_INTERNAL_CALL(GetKeyUp);
		ADD_INTERNAL_CALL(GetKey);

		LoadMonoScripts();
	}

	void Scripting::Reload() {
		s_monoScriptDomain.reset();
		s_hasComponentFuncs.clear();
		s_monoScriptObjects.clear();

		LoadMonoScripts();
	}

	void Scripting::LoadMonoScripts() {
		s_monoScriptDomain = CreateScope<MonoScriptDomain>();
		s_monoScriptDomain->SetToCurrent();

		s_monoScriptDomain->AddMonoAssembly("Resources/Scripts/Flaw-ScriptCore.dll", true);
		s_monoScriptDomain->PrintMonoAssemblyInfo(0);
		s_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 0);

		auto& projectConfig = Project::GetConfig();
		std::string projectMonoAssemblyPath = fmt::format("{}/Binaries/{}.dll", projectConfig.path, projectConfig.name);

		s_monoScriptDomain->AddMonoAssembly(projectMonoAssemblyPath.c_str(), true);
		s_monoScriptDomain->PrintMonoAssemblyInfo(1);
		s_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 1);

		// Register components
		RegisterComponent<TransformComponent>();
		RegisterComponent<SpriteRendererComponent>();
		RegisterComponent<NativeScriptComponent>();
		RegisterComponent<MonoScriptComponent>();
		RegisterComponent<Rigidbody2DComponent>();
		RegisterComponent<BoxCollider2DComponent>();
		RegisterComponent<CircleCollider2DComponent>();
		RegisterComponent<CameraComponent>();

		s_scriptAsmWatcher = CreateScope<filewatch::FileWatch<std::string>>(
			projectMonoAssemblyPath.c_str(),
			[projectMonoAssemblyPath](const std::string& path, filewatch::Event ev) {
				if (ev == filewatch::Event::modified) {
					static std::filesystem::file_time_type lastWriteTime;

					auto current = std::filesystem::last_write_time(projectMonoAssemblyPath);

					if (current - lastWriteTime >= std::chrono::milliseconds(1000)) {
						s_app->AddTask(Scripting::Reload);
					}

					lastWriteTime = current;
				}
			}
		);
	}

	void Scripting::Cleanup() {
		s_monoScriptDomain.reset();

		MonoScripting::Cleanup();
	}

	void Scripting::OnStart(Scene* scene) {
		s_scene = scene;

		auto& registry = s_scene->GetRegistry();
		for (auto&& [entity, scriptComp] : registry.view<MonoScriptComponent>().each()) {
			Entity entt(entity, s_scene);

			auto obj = CreateMonoScriptObject(entt, scriptComp.name.c_str());
			if (obj) {
				obj->CallMethod("OnCreate");
				obj->SaveMethod("OnUpdate", 0, 0);

				s_monoScriptObjects[entity] = obj;
			}
		}

		for (auto&& [entity, scriptComp] : registry.view<MonoScriptComponent>().each()) {
			auto it = s_monoScriptObjects.find(entity);
			if (it != s_monoScriptObjects.end()) {
				it->second->CallMethod("OnStart");
			}
		}
	}

	void Scripting::OnUpdate() {
		for (auto& [entity, obj] : s_monoScriptObjects) {
			obj->CallMethod(0);
		}
	}

	void Scripting::OnEnd() {
		for (auto&& [entity, scriptComp] : s_scene->GetRegistry().view<MonoScriptComponent>().each()) {
			auto it = s_monoScriptObjects.find(entity);
			if (it != s_monoScriptObjects.end()) {
				it->second->CallMethod("OnDestroy");
			}
		}
		s_monoScriptObjects.clear();

		s_scene = nullptr;
	}

	Ref<MonoScriptObject> Scripting::CreateMonoScriptObject(const Entity& entity, const char* name) {
		if (!s_monoScriptDomain->IsClassExists(name)) {
			Log::Error("Mono script class not found: %s", name);
			return nullptr;
		}

		Ref<MonoScriptObject> obj = s_monoScriptDomain->CreateInstance(name);

		// call constructor
		uint32_t enttID = (uint32_t)entity;
		void* args[] = { &enttID };
		obj->CallMethod(".ctor", args, 1);

		return obj;
	}

	Ref<MonoScriptObject> Scripting::GetMonoScriptObject(const Entity& entity) {
		entt::entity enttID = (entt::entity)(uint32_t)entity;
		auto it = s_monoScriptObjects.find(enttID);
		if (it != s_monoScriptObjects.end()) {
			return it->second;
		}
		return nullptr;
	}
}