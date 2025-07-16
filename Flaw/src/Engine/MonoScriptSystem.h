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

	class MonoComponentRuntime {
	public:
		virtual ~MonoComponentRuntime() = default;

		virtual MonoScriptObject& GetScriptObject() = 0;
	};

	class MonoEngineComponentRuntime : public MonoComponentRuntime {
	public:
		MonoEngineComponentRuntime() = default;
		MonoEngineComponentRuntime(const UUID& uuid, const char* name);
		~MonoEngineComponentRuntime() = default;

		MonoScriptObject& GetScriptObject() override { return _scriptObject; }

	private:
		friend class MonoEntity;

		MonoScriptObject _scriptObject;
	};

	class MonoProjectComponentRuntime : public MonoComponentRuntime {
	public:
		MonoProjectComponentRuntime()
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

		MonoProjectComponentRuntime(const UUID& uuid, const char* name);
		~MonoProjectComponentRuntime() = default;

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

		Ref<MonoComponentRuntime> AddComponent(const char* name);
		void RemoveComponent(const char* name);
		bool HasComponent(const char* name) const;

		Ref<MonoComponentRuntime> GetComponent(const char* name);

		template <typename T>
		Ref<T> GetComponent(const char* name) {
			return std::dynamic_pointer_cast<T>(GetComponent(name));
		}

		const UUID& GetUUID() const { return _uuid; }

		MonoScriptObject& GetScriptObject() { return _scriptObject; }

		// TODO: think about better way to get all project components
		std::unordered_set<Ref<MonoProjectComponentRuntime>> GetProjectComponents() const {
			return _projectComponents;
		}

	private:
		UUID _uuid;

		MonoScriptObject _scriptObject;

		std::unordered_map<std::string, Ref<MonoComponentRuntime>> _components;
		std::unordered_set<Ref<MonoProjectComponentRuntime>> _projectComponents;
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

		void BeginDeferredInitComponents();
		void EndDeferredInitComponents();

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

			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				return;
			}

			auto monoEntt = it->second;

			std::string monoTypeName = "Flaw." + std::string(TypeName<T>());
			monoEntt->AddComponent(monoTypeName.c_str());
		}

		template<typename T>
		void UnregisterComponent(entt::registry& registry, entt::entity entity) {
			auto& enttComp = registry.get<EntityComponent>(entity);
			auto monoEntt = GetMonoEntity(enttComp.uuid);
			if (!monoEntt) {
				return;
			}

			std::string monoTypeName = "Flaw." + std::string(TypeName<T>());

			monoEntt->RemoveComponent(monoTypeName.c_str());
		}

		void RegisterMonoComponent(entt::registry& registry, entt::entity entity);
		void RegisterMonoComponentInRuntime(entt::registry& registry, entt::entity entity);
		void UnregisterMonoComponent(entt::registry& registry, entt::entity entity);

		void SetAllComponentFields(MonoProjectComponentRuntime& monoProjectComp, MonoScriptComponent& monoScriptComp);

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

		struct DefferedInitComponentEntry {
			Ref<MonoEntity> monoEntity;
			Ref<MonoProjectComponentRuntime> projectComponent;
		};

		bool _defferedInitComponents;
		std::vector<DefferedInitComponentEntry> _deferredInitComponents;
	};
}