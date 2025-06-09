#include "pch.h"
#include "Scripting.h"
#include "Log/Log.h"
#include "Components.h"
#include "Scene.h"
#include "Platform/FileWatch.h"
#include "Application.h"
#include "Project.h"
#include "MonoInternalCall.h"
#include "Entity.h"
#include "Time/Time.h"

#include <mono/jit/jit.h>
#include <mono/metadata/reflection.h>
#include <fmt/format.h>

namespace flaw {
	#define ADD_INTERNAL_CALL(func) MonoScripting::RegisterInternalCall("Flaw.InternalCalls::"#func, func)

	static Scope<filewatch::FileWatch<std::string>> g_scriptAsmWatcher;

	static Scope<MonoScriptDomain> g_monoScriptDomain;
	static std::unordered_map<MonoType*, std::function<bool(const Entity&)>> g_hasComponentFuncs;

	static Application* g_app;
	static Scene* g_scene;
	static float g_timeSinceStart = 0.f;
	static std::unordered_map<UUID, Ref<MonoScriptObject>> g_monoRuntimeObjects;
	static std::unordered_map<UUID, Ref<MonoScriptObject>> g_monoTempObjects;

	template <typename T>
	static void RegisterComponent() {
		const std::string fullName = fmt::format("Flaw.{}", TypeName<T>());
		if (g_monoScriptDomain->IsClassExists(fullName.c_str())) {
			auto type = g_monoScriptDomain->GetClass(fullName.c_str())->GetMonoType();
			g_hasComponentFuncs[type] = [](const Entity& entity) { return entity.HasComponent<T>(); };
		}
	}

	void Scripting::Init(Application& app) {
		g_app = &app;

		MonoScripting::Init();

		// Register internal calls
		ADD_INTERNAL_CALL(LogInfo);
		ADD_INTERNAL_CALL(GetDeltaTime);
		ADD_INTERNAL_CALL(GetTimeSinceStart);
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
		g_monoScriptDomain.reset();
		g_hasComponentFuncs.clear();
		g_monoRuntimeObjects.clear();
		g_monoTempObjects.clear();

		LoadMonoScripts();
	}

	void Scripting::LoadMonoScripts() {
		g_monoScriptDomain = CreateScope<MonoScriptDomain>();
		g_monoScriptDomain->SetToCurrent();

		g_monoScriptDomain->AddMonoAssembly("Resources/Scripts/Flaw-ScriptCore.dll", true);
		g_monoScriptDomain->PrintMonoAssemblyInfo(0);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 0);
		g_monoScriptDomain->AddAllAttributesOf(0, "Flaw");

		auto& projectConfig = Project::GetConfig();
		std::string projectMonoAssemblyPath = fmt::format("{}/Binaries/{}.dll", projectConfig.path, projectConfig.name);

		g_monoScriptDomain->AddMonoAssembly(projectMonoAssemblyPath.c_str(), true);
		g_monoScriptDomain->PrintMonoAssemblyInfo(1);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 1);

		// Register components
		RegisterComponent<TransformComponent>();
		RegisterComponent<SpriteRendererComponent>();
		RegisterComponent<NativeScriptComponent>();
		RegisterComponent<MonoScriptComponent>();
		RegisterComponent<Rigidbody2DComponent>();
		RegisterComponent<BoxCollider2DComponent>();
		RegisterComponent<CircleCollider2DComponent>();
		RegisterComponent<CameraComponent>();

		g_scriptAsmWatcher = CreateScope<filewatch::FileWatch<std::string>>(
			projectMonoAssemblyPath.c_str(),
			[projectMonoAssemblyPath](const std::string& path, filewatch::Event ev) {
				if (ev == filewatch::Event::modified) {
					static std::filesystem::file_time_type lastWriteTime;

					auto current = std::filesystem::last_write_time(projectMonoAssemblyPath);

					if (current - lastWriteTime >= std::chrono::milliseconds(1000)) {
						g_app->AddTask(Scripting::Reload);
					}

					lastWriteTime = current;
				}
			}
		);
	}

	void Scripting::Cleanup() {
		g_monoScriptDomain.reset();

		MonoScripting::Cleanup();
	}

	void Scripting::OnStart(Scene* scene) {
		g_scene = scene;
		g_timeSinceStart = 0.f;

		auto& registry = g_scene->GetRegistry();
		for (auto&& [entity, entityComp, scriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto obj = CreateMonoScriptObjectImpl(entityComp.uuid, scriptComp.name.c_str());
			if (obj) {
				obj->CallMethod("OnCreate");
				obj->SaveMethod("OnUpdate", 0, 0);

				g_monoRuntimeObjects[entityComp.uuid] = obj;
			}

			auto it = g_monoTempObjects.find(entityComp.uuid);
			if (it == g_monoTempObjects.end()) {
				continue;
			}

			auto tempObj = it->second;
			auto objClass = obj->GetClass();
			auto tempObjClass = tempObj->GetClass();

			if (objClass != tempObjClass) {
				Log::Error("Mono script class mismatch for entity %d: %s vs %s", (uint32_t)entity, obj->GetClass()->GetTypeName().data(), tempObj->GetClass()->GetTypeName().data());
				continue;
			}

			std::array<int8_t, 1024> buff;
			objClass->EachPublicFields([&obj, &tempObj, &buff](std::string_view fieldName, MonoScriptClassField& field) {
				field.GetValue(tempObj.get(), buff.data());
				field.SetValue(obj.get(), buff.data());
			});
		}

		for (auto&& [entity, entityComp, scriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it != g_monoRuntimeObjects.end()) {
				it->second->CallMethod("OnStart");
			}
		}
	}

	void Scripting::OnUpdate() {
		for (auto& [entity, entityComp, scriptComp] : g_scene->GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it != g_monoRuntimeObjects.end()) {
				it->second->CallMethod(0); // Call the saved method at slot 0
			}
		}

		// TODO: how to handle removed objects and created objects on runtime?

		g_timeSinceStart += Time::DeltaTime();
	}

	void Scripting::OnEnd() {
		for (auto&& [entity, entityComp, scriptComp] : g_scene->GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it != g_monoRuntimeObjects.end()) {
				it->second->CallMethod("OnDestroy");
			}
		}
		g_monoRuntimeObjects.clear();

		g_scene = nullptr;
	}

	Ref<MonoScriptObject> Scripting::CreateTempMonoScriptObject(const UUID& uuid, const char* name) {
		auto obj = CreateMonoScriptObjectImpl(uuid, name);
		if (!obj) {
			return nullptr;
		}
		g_monoTempObjects[uuid] = obj;
		return obj;
	}

	void Scripting::DestroyTempMonoScriptObject(const UUID& uuid) {
		auto it = g_monoTempObjects.find(uuid);
		if (it != g_monoTempObjects.end()) {
			g_monoTempObjects.erase(it);
		}
	}

	Ref<MonoScriptObject> Scripting::CreateMonoScriptObjectImpl(const UUID& uuid, const char* name) {
		if (!g_monoScriptDomain->IsClassExists(name)) {
			Log::Error("Mono script class not found: %s", name);
			return nullptr;
		}

		Ref<MonoScriptObject> obj = g_monoScriptDomain->CreateInstance(name);

		// call constructor
		UUID copy = uuid;
		void* args[] = { &copy };
		obj->CallMethod(".ctor", args, 1);

		return obj;
	}

	Ref<MonoScriptObject> Scripting::GetTempMonoScriptObject(const UUID& uuid) {
		auto it = g_monoTempObjects.find(uuid);
		if (it != g_monoTempObjects.end()) {
			return it->second;
		}
		return nullptr;
	}

	bool Scripting::IsEngineComponent(MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);
		return g_hasComponentFuncs.find(monoType) != g_hasComponentFuncs.end();
	}

	bool Scripting::HasComponent(UUID uuid, MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);
		FASSERT(g_hasComponentFuncs.find(monoType) != g_hasComponentFuncs.end(), "Component type not found");
		auto entity = g_scene->FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");
		return g_hasComponentFuncs.at(monoType)(entity);
	}

	bool Scripting::IsComponentInstanceExists(UUID uuid) {
		return g_monoRuntimeObjects.find(uuid) != g_monoRuntimeObjects.end();
	}

	MonoObject* Scripting::GetComponentInstance(UUID uuid) {
		FASSERT(IsComponentInstanceExists(uuid), "Component instance not found");
		return g_monoRuntimeObjects[uuid]->GetMonoObject();
	}

	MonoScriptDomain& Scripting::GetMonoScriptDomain() {
		return *g_monoScriptDomain;
	}

	Application& Scripting::GetApplication() {
		return *g_app;
	}

	Scene& Scripting::GetScene() {
		return *g_scene;
	}

	float Scripting::GetTimeSinceStart() {
		return g_timeSinceStart;
	}
}