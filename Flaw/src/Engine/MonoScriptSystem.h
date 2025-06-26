#pragma once

#include "Core.h"
#include "Scripting.h"
#include "Utils/UUID.h"
#include "Entity.h"
#include "Assets.h"
#include "Physics.h"

namespace flaw {
	class Application;
	class Scene;
	class MonoScriptComponent;

	class MonoComponentInstance {
	public:
		virtual ~MonoComponentInstance() = default;

		virtual MonoScriptObject& GetScriptObject() = 0;
	};

	class MonoEngineComponentInstance : public MonoComponentInstance {
	public:
		MonoEngineComponentInstance() = default;
		MonoEngineComponentInstance(const UUID& uuid, const char* name);
		~MonoEngineComponentInstance() = default;

		MonoScriptObject& GetScriptObject() override { return _scriptObject; }

	private:
		friend class MonoEntity;

		MonoScriptObject _scriptObject;
	};

	class MonoProjectComponentInstance : public MonoComponentInstance {
	public:
		MonoProjectComponentInstance()
			: _createMethod(nullptr)
			, _startMethod(nullptr)
			, _updateMethod(nullptr)
			, _destroyMethod(nullptr)
			, _onCollisionEnterMethod(nullptr)
			, _onCollisionStayMethod(nullptr)
			, _onCollisionExitMethod(nullptr)
			, _onTriggerEnterMethod(nullptr)
			, _onTriggerStayMethod(nullptr)
			, _onTriggerExitMethod(nullptr) 
		{}

		MonoProjectComponentInstance(const UUID& uuid, const char* name);
		~MonoProjectComponentInstance() = default;

		void CallOnCreate() const;
		void CallOnStart() const;
		void CallOnUpdate() const;
		void CallOnDestroy() const;
		void CallOnCollisionEnter(MonoScriptObject& collisionInfo) const;
		void CallOnCollisionStay(MonoScriptObject& collisionInfo) const;
		void CallOnCollisionExit(MonoScriptObject& collisionInfo) const;

		MonoScriptObject& GetScriptObject() override { return _scriptObject; }

	private:
		static void CreatePublicFieldsInObjectRecursive(MonoScriptObjectView& object);
		
	private:
		friend class MonoEntity;

		MonoScriptObject _scriptObject;

		MonoMethod* _createMethod;
		MonoMethod* _startMethod;
		MonoMethod* _updateMethod;
		MonoMethod* _destroyMethod;
		MonoMethod* _onCollisionEnterMethod;
		MonoMethod* _onCollisionStayMethod;
		MonoMethod* _onCollisionExitMethod;
		MonoMethod* _onTriggerEnterMethod;
		MonoMethod* _onTriggerStayMethod;
		MonoMethod* _onTriggerExitMethod;
	};

	class MonoEntity {
	public:
		MonoEntity(const UUID& uuid);

		MonoComponentInstance* AddComponent(const char* name);
		void RemoveComponent(const char* name);
		MonoComponentInstance* GetComponent(const char* name);
		MonoProjectComponentInstance* GetProjectComponent(const char* name);
		bool HasComponent(const char* name) const;

		MonoScriptObject& GetScriptObject() { return _scriptObject; }

		std::unordered_map<std::string, MonoProjectComponentInstance>& GetProjectComponents() { return _projectComponents; }

	private:
		UUID _uuid;

		MonoScriptObject _scriptObject;

		std::unordered_map<std::string, MonoEngineComponentInstance> _engineComponents;
		std::unordered_map<std::string, MonoProjectComponentInstance> _projectComponents;
	};

	class MonoAsset {
	public:
		MonoAsset(const AssetHandle& handle, const Ref<Asset>& asset);

		MonoScriptObject& GetScriptObject() { return _scriptObject; }

	private:
		MonoScriptObject _scriptObject;
	};

	class MonoScriptSystem {
	public:
		MonoScriptSystem(Application& app, Scene& scene);

		void Start();
		void Update();
		void End();

		bool IsMonoEntityExists(const UUID& uuid) const;
		Ref<MonoEntity> GetMonoEntity(const UUID& uuid) const;

		Scene& GetScene() const { return _scene; }
		float GetTimeSinceStart() const { return _timeSinceStart; }

	private:
		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		template<typename T>
		void RegisterComponent(entt::registry& registry, entt::entity entity) {
			if (!registry.any_of<T>(entity)) {
				return;
			}

			auto& enttComp = registry.get<EntityComponent>(entity);
			auto monoEntt = GetMonoEntity(enttComp.uuid);
			if (!monoEntt) {
				return;
			}

			std::string monoTypeName;
			if constexpr (std::is_same_v<T, MonoScriptComponent>) {
				auto& monoComp = registry.get<MonoScriptComponent>(entity);
				monoTypeName = monoComp.name;
			}
			else {
				monoTypeName = "Flaw." + std::string(TypeName<T>());
			}

			if (monoEntt->HasComponent(monoTypeName.c_str())) {
				return;
			}

			monoEntt->AddComponent(monoTypeName.c_str());
		}

		void RegisterMonoComponentInRuntime(entt::registry& registry, entt::entity entity);

		void SetAllComponentFields(MonoProjectComponentInstance& monoProjectComp, MonoScriptComponent& monoScriptComp);

		const char* GetMonoColliderClassName(PhysicsShapeType shapeType) const;
		std::vector<MonoScriptObject> CreateContactPointObjects(const std::vector<ContactPoint>& contactPoints) const;
		MonoScriptObject CreateCollisionInfoObject(MonoScriptObject& colliderA, MonoScriptObject& colliderB, MonoScriptArray& contactArray) const;

		void HandleOnCollisionEnter(const CollisionInfo& collisionInfo) const;
		void HandleOnCollisionStay(const CollisionInfo& collisionInfo) const;
		void HandleOnCollisionExit(const CollisionInfo& collisionInfo) const;

	private:
		friend class Scripting;

		Application& _app;
		Scene& _scene;

		float _timeSinceStart = 0.f;

		std::unordered_map<AssetHandle, MonoAsset> _monoAssets;

		std::unordered_map<UUID, Ref<MonoEntity>> _monoEntities;
		std::vector<Ref<MonoEntity>> _monoEntitiesToDestroy;
	};
}