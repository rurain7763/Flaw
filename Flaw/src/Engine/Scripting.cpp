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
#include "MonoScriptSystem.h"

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
	static std::unordered_set<MonoScriptSystem*> g_monoScriptSystems;
	static MonoScriptSystem* g_activeMonoScriptSys;

	template <typename T>
	static void RegisterComponent() {
		const std::string fullName = fmt::format("Flaw.{}", TypeName<T>());
		if (g_monoScriptDomain->IsClassExists(fullName.c_str())) {
			auto type = g_monoScriptDomain->GetClass(fullName.c_str()).GetMonoType();
			g_hasComponentFuncs[type] = [](const Entity& entity) { return entity.HasComponent<T>(); };
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

	void Scripting::Init(Application& app) {
		g_app = &app;

		MonoScripting::Init();

		// Register internal calls
		ADD_INTERNAL_CALL(IsEngineComponent);
		ADD_INTERNAL_CALL(HasComponent);
		ADD_INTERNAL_CALL(IsComponentInstanceExists);
		ADD_INTERNAL_CALL(GetComponentInstance);
		ADD_INTERNAL_CALL(GetTimeSinceStart);
		ADD_INTERNAL_CALL(LogInfo);
		ADD_INTERNAL_CALL(GetDeltaTime);
		ADD_INTERNAL_CALL(DestroyEntity);
		ADD_INTERNAL_CALL(FindEntityByName);
		ADD_INTERNAL_CALL(CreateEntity_Prefab);
		ADD_INTERNAL_CALL(CreateEntityWithTransform_Prefab);
		ADD_INTERNAL_CALL(GetPosition_Transform);
		ADD_INTERNAL_CALL(SetPosition_Transform);
		ADD_INTERNAL_CALL(GetRotation_Transform);
		ADD_INTERNAL_CALL(SetRotation_Transform);
		ADD_INTERNAL_CALL(GetScale_Transform);
		ADD_INTERNAL_CALL(SetScale_Transform);
		ADD_INTERNAL_CALL(GetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(SetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(GetLinearVelocity_RigidBody2D);
		ADD_INTERNAL_CALL(GetKeyDown_Input);
		ADD_INTERNAL_CALL(GetKeyUp_Input);
		ADD_INTERNAL_CALL(GetKey_Input);

		LoadMonoScripts();
	}

	void Scripting::Reload() {
		using MonoScriptInstanceBackUpTrees = std::vector<std::pair<UUID, Ref<MonoScriptObjectTreeNode>>>;

		std::unordered_map<MonoScriptSystem*, MonoScriptInstanceBackUpTrees> backupTrees;
		for (auto* system : g_monoScriptSystems) {
			MonoScriptInstanceBackUpTrees backupNodes;
			for (const auto& [uuid, instance] : system->_monoInstances) {
				backupNodes.push_back({ uuid, instance.scriptObject.ToTree() });
			}
			backupTrees[system] = std::move(backupNodes);
			system->_monoInstances.clear();
		}

		g_hasComponentFuncs.clear();
		LoadMonoScripts();

		for (auto* system : g_monoScriptSystems) {
			auto& backupNodes = backupTrees[system];
			for (const auto& [uuid, backupNode] : backupNodes) {
				std::string typeName = backupNode->typeName;
				if (!system->CreateMonoScriptInstance(uuid, typeName.c_str())) {
					continue;
				}
				system->GetMonoScriptInstance(uuid).scriptObject.ApplyTree(backupNode);
			}			
		}
	}

	void Scripting::Cleanup() {
		g_monoScriptDomain.reset();

		MonoScripting::Cleanup();
	}

	void Scripting::RegisterMonoScriptSystem(MonoScriptSystem* system) {
		g_monoScriptSystems.insert(system);
	}

	void Scripting::UnregisterMonoScriptSystem(MonoScriptSystem* system) {
		g_monoScriptSystems.erase(system);
	}

	void Scripting::SetActiveMonoScriptSystem(MonoScriptSystem* system) {
		g_activeMonoScriptSys = system;
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
		auto entity = g_activeMonoScriptSys->GetScene().FindEntityByUUID(uuid);
		FASSERT(entity, "Entity not found with UUID");
		return g_hasComponentFuncs.at(monoType)(entity);
	}

	bool Scripting::IsComponentInstanceExists(UUID uuid) {
		return g_activeMonoScriptSys->IsMonoScriptInstanceExists(uuid);
	}

	MonoObject* Scripting::GetComponentInstance(UUID uuid) {
		return g_activeMonoScriptSys->GetMonoScriptInstance(uuid).scriptObject.GetMonoObject();
	}

	float Scripting::GetDeltaTime() {
		return Time::DeltaTime();
	}

	float Scripting::GetTimeSinceStart() {
		return g_activeMonoScriptSys->GetTimeSinceStart();
	}

	MonoScriptDomain& Scripting::GetMonoScriptDomain() {
		return *g_monoScriptDomain;
	}

	Application& Scripting::GetApplication() {
		return *g_app;
	}

	Scene& Scripting::GetScene() {
		return g_activeMonoScriptSys->GetScene();
	}
}