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

	struct MonoRuntimeObject {
		Ref<MonoScriptObject> scriptObject;
		MonoMethod* updateMethod;
	};

	static Scope<filewatch::FileWatch<std::string>> g_scriptAsmWatcher;

	static Scope<MonoScriptDomain> g_monoScriptDomain;
	static std::unordered_map<MonoType*, std::function<bool(const Entity&)>> g_hasComponentFuncs;

	static Application* g_app;
	static Scene* g_scene;
	static float g_timeSinceStart = 0.f;
	static std::unordered_map<UUID, MonoRuntimeObject> g_monoRuntimeObjects;
	static std::unordered_map<UUID, Ref<MonoScriptObject>> g_monoTempObjects;

	template <typename T>
	static void RegisterComponent() {
		const std::string fullName = fmt::format("Flaw.{}", TypeName<T>());
		if (g_monoScriptDomain->IsClassExists(fullName.c_str())) {
			auto type = g_monoScriptDomain->GetClass(fullName.c_str()).GetMonoType();
			g_hasComponentFuncs[type] = [](const Entity& entity) { return entity.HasComponent<T>(); };
		}
	}

	void Scripting::Init(Application& app) {
		g_app = &app;

		MonoScripting::Init();

		// Register internal calls
		ADD_INTERNAL_CALL(IsEngineComponent);
		ADD_INTERNAL_CALL(HasComponent);
		ADD_INTERNAL_CALL(IsComponentInstanceExists);
		ADD_INTERNAL_CALL(GetComponentInstance);
		ADD_INTERNAL_CALL(LogInfo);
		ADD_INTERNAL_CALL(GetDeltaTime);
		ADD_INTERNAL_CALL(GetTimeSinceStart);
		ADD_INTERNAL_CALL(DestroyEntity);
		ADD_INTERNAL_CALL(FindEntityByName);
		ADD_INTERNAL_CALL(CreateEntity_Prefab);
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
		std::vector<std::pair<UUID, Ref<MonoScriptObjectTreeNode>>> backupNodes;
		for (const auto& [uuid, obj] : g_monoTempObjects) {
			backupNodes.push_back({ uuid, obj->ToTree() });
		}

		g_monoTempObjects.clear();
		g_monoRuntimeObjects.clear();
		g_hasComponentFuncs.clear();
		g_monoScriptDomain.reset();

		LoadMonoScripts();

		for (const auto& [uuid, backupNode] : backupNodes) {
			auto obj = CreateTempMonoScriptObject(uuid, backupNode->typeName.c_str());
			if (!obj) {
				continue;
			}
			obj->ApplyTree(backupNode);
		}
	}

	void Scripting::LoadMonoScripts() {
		g_monoScriptDomain = CreateScope<MonoScriptDomain>();
		g_monoScriptDomain->SetToCurrent();

		g_monoScriptDomain->AddMonoAssembly("Resources/Scripts/Flaw-ScriptCore.dll", true);
		g_monoScriptDomain->PrintMonoAssemblyInfo(0);
		g_monoScriptDomain->AddAllSubClassesOf("System", "Attribute", 0);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 0);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "Asset", 0);

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

	static void CreatePublicFieldsInObjectRecursive(MonoScriptObject& object) {
		object.GetClass().EachFields([&object](std::string_view fieldName, MonoScriptClassField& field) {
			if (field.IsClass()) {
				MonoScriptObject fieldMonoObj(object.GetMonoDomain(), field.GetMonoClass(), field.GetValue<MonoObject*>(&object));

				if (!fieldMonoObj.IsValid()) {
					fieldMonoObj.Instantiate();
					field.SetValue(&object, fieldMonoObj.GetMonoObject());
				}

				CreatePublicFieldsInObjectRecursive(fieldMonoObj);
			}
		});
	}

	Ref<MonoScriptObject> Scripting::CreateMonoScriptObjectImpl(const UUID& uuid, const char* name) {
		if (!g_monoScriptDomain->IsClassExists(name)) {
			Log::Error("Mono script class not found: %s", name);
			return nullptr;
		}

		UUID copy = uuid;
		void* args[] = { &copy };

		auto instance = g_monoScriptDomain->CreateInstance(name, args, 1);
		CreatePublicFieldsInObjectRecursive(instance);

		return CreateRef<MonoScriptObject>(instance);
	}

	void Scripting::OnStart(Scene* scene) {
		g_scene = scene;
		g_timeSinceStart = 0.f;

		auto& registry = g_scene->GetRegistry();
		for (auto&& [entity, entityComp, scriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			Ref<MonoScriptObject> runtimeObj;

			auto it = g_monoTempObjects.find(entityComp.uuid);
			if (it == g_monoTempObjects.end()) {
				runtimeObj = CreateMonoScriptObjectImpl(entityComp.uuid, scriptComp.name.c_str());
			}
			else {
				runtimeObj = CreateRef<MonoScriptObject>(it->second->Clone());
			}

			if (!runtimeObj) {
				continue;
			}

			auto runtimeClass = runtimeObj->GetClass();
			auto createMethod = runtimeClass.GetMethodRecurcive("OnCreate", 0);
			auto updateMethod = runtimeClass.GetMethodRecurcive("OnUpdate", 0);

			g_monoRuntimeObjects[entityComp.uuid] = { runtimeObj, updateMethod };

			if (createMethod) {
				runtimeObj->CallMethod(createMethod);
			}
		}

		for (auto&& [entity, entityComp, scriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it == g_monoRuntimeObjects.end()) {
				continue;
			}

			auto objClass = it->second.scriptObject->GetClass();
			auto startMethod = objClass.GetMethodRecurcive("OnStart", 0);
			if (startMethod) {
				it->second.scriptObject->CallMethod(startMethod);
			}
		}
	}

	void Scripting::OnUpdate() {
		for (auto& [entity, entityComp, scriptComp] : g_scene->GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it == g_monoRuntimeObjects.end()) {
				continue;
			}

			if (it->second.updateMethod) {
				it->second.scriptObject->CallMethod(it->second.updateMethod);
			}
		}

		// TODO: how to handle removed objects and created objects on runtime?

		g_timeSinceStart += Time::DeltaTime();
	}

	void Scripting::OnEnd() {
		for (auto&& [entity, entityComp, scriptComp] : g_scene->GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = g_monoRuntimeObjects.find(entityComp.uuid);
			if (it == g_monoRuntimeObjects.end()) {
				continue;
			}

			auto objClass = it->second.scriptObject->GetClass();
			auto destroyMethod = objClass.GetMethodRecurcive("OnDestroy", 0);
			if (destroyMethod) {
				it->second.scriptObject->CallMethod(destroyMethod);
			}
		}
		g_monoRuntimeObjects.clear();

		g_scene = nullptr;
	}

	MonoScriptClass& Scripting::GetMonoSystemClass(MonoSystemType type) {
		return g_monoScriptDomain->GetSystemClass(type);
	}

	MonoScriptClass& Scripting::GetMonoAssetClass(MonoAssetType type) {
		switch (type) {
		case MonoAssetType::Prefab:
			return g_monoScriptDomain->GetClass("Flaw.Prefab");
		}
	}

	MonoScriptClass& Scripting::GetMonoClass(const char* name) {
		return g_monoScriptDomain->GetClass(name);
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

	Ref<MonoScriptObject> Scripting::GetTempMonoScriptObject(const UUID& uuid) {
		auto it = g_monoTempObjects.find(uuid);
		if (it != g_monoTempObjects.end()) {
			return it->second;
		}
		return nullptr;
	}

	Ref<MonoScriptObject> Scripting::CreateRuntimeMonoScriptObject(const UUID& uuid, const char* name) {
		auto obj = CreateMonoScriptObjectImpl(uuid, name);
		if (!obj) {
			return nullptr;
		}

		auto objClass = obj->GetClass();
		auto createMethod = objClass.GetMethodRecurcive("OnCreate", 0);
		auto startMethod = objClass.GetMethodRecurcive("OnStart", 0);
		auto updateMethod = objClass.GetMethodRecurcive("OnUpdate", 0);

		if (createMethod) {
			obj->CallMethod(createMethod);
		}

		if (startMethod) {
			obj->CallMethod(startMethod);
		}

		g_monoRuntimeObjects[uuid] = { obj, updateMethod };

		return obj;
	}

	void Scripting::DestroyRuntimeMonoScriptObject(const UUID& uuid) {
		auto it = g_monoRuntimeObjects.find(uuid);
		if (it != g_monoRuntimeObjects.end()) {
			auto objClass = it->second.scriptObject->GetClass();
			auto destroyMethod = objClass.GetMethodRecurcive("OnDestroy", 0);
			if (destroyMethod) {
				it->second.scriptObject->CallMethod(destroyMethod);
			}
			g_monoRuntimeObjects.erase(it);
		}
	}

	bool Scripting::IsMonoComponent(const MonoScriptClass& monoClass) {
		auto entityComponentMonoClass = g_monoScriptDomain->GetClass("Flaw.EntityComponent");
		return entityComponentMonoClass != monoClass && monoClass.IsSubClassOf(&entityComponentMonoClass);
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
		return g_monoRuntimeObjects[uuid].scriptObject->GetMonoObject();
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