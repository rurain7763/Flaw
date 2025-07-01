#include "pch.h"
#include "Scripting.h"
#include "Log/Log.h"
#include "Components.h"
#include "Scene.h"
#include "Platform/FileWatch.h"
#include "Application.h"
#include "Project.h"
#include "MonoInternalCall.h"
#include "Time/Time.h"
#include "MonoScriptSystem.h"

#include <mono/jit/jit.h>
#include <mono/metadata/reflection.h>
#include <fmt/format.h>

namespace flaw {
	#define ADD_INTERNAL_CALL(func) g_monoScriptContext->RegisterInternalCall("Flaw.InternalCalls::"#func, func)

	struct EngineComponentConverter {
		std::function<bool(const Entity&)> hasComponentFunc;
	};

	static Scope<filewatch::FileWatch<std::string>> g_scriptAsmWatcher;

	static Scope<MonoScriptContext> g_monoScriptContext;
	static Ref<MonoScriptDomain> g_monoScriptDomain;
	static std::unordered_map<MonoType*, EngineComponentConverter> g_engineComponentConverters;

	static Application* g_app;
	static MonoScriptSystem* g_activeMonoScriptSys;

	template <typename T>
	static void RegisterEngineComponent() {
		const std::string fullName = fmt::format("Flaw.{}", TypeName<T>());
		if (g_monoScriptDomain->IsClassExists(fullName.c_str())) {
			auto type = g_monoScriptDomain->GetClass(fullName.c_str()).GetMonoType();

			EngineComponentConverter engineComp;
			engineComp.hasComponentFunc = [](const Entity& entity) { return entity.HasComponent<T>(); };

			g_engineComponentConverters[type] = engineComp;
		}
	}

	void Scripting::LoadMonoScripting() {
		g_monoScriptDomain = g_monoScriptContext->CreateDomain();
		g_monoScriptContext->SetCurrentDomain(g_monoScriptDomain);

		g_monoScriptDomain->AddMonoAssembly("Resources/Scripts/Flaw-ScriptCore.dll", true);
		g_monoScriptDomain->PrintMonoAssemblyInfo(0);
		g_monoScriptDomain->AddClass("Flaw", "Entity", 0);
		g_monoScriptDomain->AddClass("Flaw", "ContactPoint", 0);
		g_monoScriptDomain->AddClass("Flaw", "CollisionInfo", 0);
		g_monoScriptDomain->AddClass("Flaw", "TriggerInfo", 0);
		g_monoScriptDomain->AddAllSubClassesOf("System", "Attribute", 0);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 0);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "Asset", 0);

		auto& projectConfig = Project::GetConfig();
		std::string projectMonoAssemblyPath = fmt::format("{}/Binaries/{}.dll", projectConfig.path, projectConfig.name);

		g_monoScriptDomain->AddMonoAssembly(projectMonoAssemblyPath.c_str(), true);
		g_monoScriptDomain->PrintMonoAssemblyInfo(1);
		g_monoScriptDomain->AddAllSubClassesOf(0, "Flaw", "EntityComponent", 1);

		// Register components
		RegisterEngineComponent<TransformComponent>();
		RegisterEngineComponent<SpriteRendererComponent>();
		RegisterEngineComponent<NativeScriptComponent>();
		RegisterEngineComponent<MonoScriptComponent>();
		RegisterEngineComponent<Rigidbody2DComponent>();
		RegisterEngineComponent<BoxCollider2DComponent>();
		RegisterEngineComponent<CircleCollider2DComponent>();
		RegisterEngineComponent<RigidbodyComponent>();
		RegisterEngineComponent<BoxColliderComponent>();
		RegisterEngineComponent<SphereColliderComponent>();
		RegisterEngineComponent<MeshColliderComponent>();
		RegisterEngineComponent<CameraComponent>();
		RegisterEngineComponent<AnimatorComponent>();
		RegisterEngineComponent<SkeletalMeshComponent>();

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

		g_monoScriptContext = CreateScope<MonoScriptContext>();

		// Register internal calls
		ADD_INTERNAL_CALL(GetEntity);
		ADD_INTERNAL_CALL(HasComponent);
		ADD_INTERNAL_CALL(GetComponentInstance);
		ADD_INTERNAL_CALL(GetTimeSinceStart);
		ADD_INTERNAL_CALL(LogInfo);
		ADD_INTERNAL_CALL(GetDeltaTime);
		ADD_INTERNAL_CALL(DestroyEntity);
		ADD_INTERNAL_CALL(FindEntityByName);
		ADD_INTERNAL_CALL(CreateEntity_Prefab);
		ADD_INTERNAL_CALL(CreateEntityWithTransform_Prefab);
		ADD_INTERNAL_CALL(GetEntityName_Entity);
		ADD_INTERNAL_CALL(GetPosition_Transform);
		ADD_INTERNAL_CALL(SetPosition_Transform);
		ADD_INTERNAL_CALL(GetRotation_Transform);
		ADD_INTERNAL_CALL(SetRotation_Transform);
		ADD_INTERNAL_CALL(GetScale_Transform);
		ADD_INTERNAL_CALL(SetScale_Transform);
		ADD_INTERNAL_CALL(GetForward_Transform);
		ADD_INTERNAL_CALL(GetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(SetBodyType_RigidBody2D);
		ADD_INTERNAL_CALL(GetLinearVelocity_RigidBody2D);
		ADD_INTERNAL_CALL(ScreenToWorld_Camera);
		ADD_INTERNAL_CALL(GetKeyDown_Input);
		ADD_INTERNAL_CALL(GetKeyUp_Input);
		ADD_INTERNAL_CALL(GetKey_Input);
		ADD_INTERNAL_CALL(GetMousePosition_Input);
		ADD_INTERNAL_CALL(GetMouseButtonDown_Input);
		ADD_INTERNAL_CALL(GetMouseButtonUp_Input);
		ADD_INTERNAL_CALL(GetMouseButton_Input);
		ADD_INTERNAL_CALL(Raycast_Physics);
		ADD_INTERNAL_CALL(PlayState_Animator);
		ADD_INTERNAL_CALL(AttachEntityToSocket_SkeletalMesh);

		LoadMonoScripting();
	}

	void Scripting::Reload() {
		g_engineComponentConverters.clear();
		LoadMonoScripting();
	}

	void Scripting::Cleanup() {
		g_monoScriptDomain.reset();
		g_monoScriptContext.reset();
	}

	void Scripting::SetActiveMonoScriptSystem(MonoScriptSystem* system) {
		g_activeMonoScriptSys = system;
	}

	void Scripting::MonoCollectGarbage() {
		g_monoScriptContext->CollectGarbage();
	}

	void Scripting::MonoPrintAllGCObjects() {
		g_monoScriptContext->PrintAllGCObjects();
	}

	MonoScriptClass& Scripting::GetMonoSystemClass(MonoSystemType type) {
		return g_monoScriptDomain->GetSystemClass(type);
	}

	MonoScriptClass& Scripting::GetMonoAssetClass(AssetType type) {
		switch (type) {
			case AssetType::GraphicsShader:
				return g_monoScriptDomain->GetClass("Flaw.GraphicsShader");
			case AssetType::Texture2D:
				return g_monoScriptDomain->GetClass("Flaw.Texture2D");
			case AssetType::Font:
				return g_monoScriptDomain->GetClass("Flaw.Font");
			case AssetType::Sound:
				return g_monoScriptDomain->GetClass("Flaw.Sound");
			case AssetType::StaticMesh:
				return g_monoScriptDomain->GetClass("Flaw.StaticMesh");
			case AssetType::TextureCube:
				return g_monoScriptDomain->GetClass("Flaw.TextureCube");
			case AssetType::Texture2DArray:
				return g_monoScriptDomain->GetClass("Flaw.Texture2DArray");
			case AssetType::SkeletalMesh:
				return g_monoScriptDomain->GetClass("Flaw.SkeletalMesh");
			case AssetType::Skeleton:
				return g_monoScriptDomain->GetClass("Flaw.Skeleton");
			case AssetType::SkeletalAnimation:
				return g_monoScriptDomain->GetClass("Flaw.SkeletalAnimation");
			case AssetType::Material:
				return g_monoScriptDomain->GetClass("Flaw.Material");
			case AssetType::Prefab:
				return g_monoScriptDomain->GetClass("Flaw.Prefab");
		}

		throw std::runtime_error("Unsupported MonoAssetType");
	}

	MonoScriptClass& Scripting::GetMonoClass(const char* name) {
		return g_monoScriptDomain->GetClass(name);
	}

	bool Scripting::HasMonoClass(const char* name) {
		return g_monoScriptDomain->IsClassExists(name);
	}

	bool Scripting::IsMonoComponent(const MonoScriptClass& monoClass) {
		auto entityComponentMonoClass = g_monoScriptDomain->GetClass("Flaw.EntityComponent");
		return entityComponentMonoClass != monoClass && monoClass.IsSubClassOf(entityComponentMonoClass);
	}

	bool Scripting::IsMonoProjectComponent(const MonoScriptClass& monoClass) {
		return IsMonoComponent(monoClass) && g_engineComponentConverters.find(monoClass.GetMonoType()) == g_engineComponentConverters.end();
	}

	bool Scripting::IsMonoAsset(const MonoScriptClass& monoClass) {
		auto assetMonoClass = g_monoScriptDomain->GetClass("Flaw.Asset");
		return assetMonoClass != monoClass && monoClass.IsSubClassOf(assetMonoClass);
	}

	bool Scripting::HasEngineComponent(const Entity& entity, const char* compName) {
		UUID uuid = entity.GetUUID();

		MonoScriptClass& monoClass = GetMonoClass(compName);
		auto it = g_engineComponentConverters.find(monoClass.GetMonoType());
		if (it != g_engineComponentConverters.end()) {
			return it->second.hasComponentFunc(entity);
		}

		return false;
	}

	void Scripting::GetMonoHeapInfo(int64_t& heapSize, int64_t& usedSize) {
		g_monoScriptContext->GetHeapInfo(heapSize, usedSize);
	}

	MonoObject* Scripting::GetEntity(UUID uuid) {
		if (!g_activeMonoScriptSys->IsMonoEntityExists(uuid)) {
			Log::Error("Mono entity with UUID %lld does not exist", uuid);
			return nullptr;
		}

		return g_activeMonoScriptSys->GetMonoEntity(uuid)->GetScriptObject().GetMonoObject();
	}

	bool Scripting::HasComponent(UUID uuid, MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);
		const char* typeName = mono_type_get_name_full(monoType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);

		if (!g_activeMonoScriptSys->IsMonoEntityExists(uuid)) {
			return false;
		}

		auto monoEntity = g_activeMonoScriptSys->GetMonoEntity(uuid);

		return monoEntity->HasComponent(typeName);
	}

	MonoObject* Scripting::GetComponentInstance(UUID uuid, MonoReflectionType* type) {
		MonoType* monoType = mono_reflection_type_get_type(type);

		if (!g_activeMonoScriptSys->IsMonoEntityExists(uuid)) {
			Log::Error("Mono entity with UUID %lld does not exist", uuid);
			return nullptr;
		}

		auto monoEntity = g_activeMonoScriptSys->GetMonoEntity(uuid);
		const char* typeName = mono_type_get_name_full(monoType, MonoTypeNameFormat::MONO_TYPE_NAME_FORMAT_FULL_NAME);
		if (!monoEntity->HasComponent(typeName)) {
			Log::Error("Mono entity with UUID %lld does not have component %s", uuid, typeName);
			return nullptr;
		}

		return monoEntity->GetComponent(typeName)->GetScriptObject().GetMonoObject();
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