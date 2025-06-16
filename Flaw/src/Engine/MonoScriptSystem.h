#pragma once

#include "Core.h"
#include "Scripting.h"
#include "Utils/UUID.h"
#include "Entity.h"
#include "Assets.h"

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

		MonoScriptObject& GetScriptObject() override { return _scriptObject; }

	private:
		friend class MonoEntity;

		MonoScriptObject _scriptObject;
	};

	class MonoProjectComponentInstance : public MonoComponentInstance {
	public:
		MonoProjectComponentInstance() = default;
		MonoProjectComponentInstance(const UUID& uuid, const char* name);

		void CallOnCreate() const;
		void CallOnStart() const;
		void CallOnUpdate() const;
		void CallOnDestroy() const;

		MonoScriptObject& GetScriptObject() override { return _scriptObject; }

	private:
		static void CreatePublicFieldsInObjectRecursive(MonoScriptObject& object);
		
	private:
		friend class MonoEntity;

		MonoScriptObject _scriptObject;

		MonoMethod* _createMethod;
		MonoMethod* _startMethod;
		MonoMethod* _updateMethod;
		MonoMethod* _destroyMethod;
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

		std::unordered_map<std::string, MonoEngineComponentInstance>& GetEngineComponents() { return _engineComponents; }
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

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Start();
		void Update();
		void End();

		bool IsMonoEntityExists(const UUID& uuid) const;
		Ref<MonoEntity> GetMonoEntity(const UUID& uuid);

		Scene& GetScene() const { return _scene; }
		float GetTimeSinceStart() const { return _timeSinceStart; }
		const std::unordered_map<UUID, Ref<MonoEntity>>& GetMonoEntities() const { return _monoEntities; }

	private:
		template<typename T>
		void RegisterComponentSyncFunc() {
			_componentSyncFuncs.emplace_back([this](Entity entity, Ref<MonoEntity> monoEntity) {
				if (entity.HasComponent<T>()) {
					auto& comp = entity.GetComponent<T>();
					std::string monoTypeName = "Flaw." + std::string(TypeName<T>());
					monoEntity->AddComponent(monoTypeName.c_str());
				}
			});
		}

		Ref<MonoEntity> CreateMonoEntityByEntity(const Entity& entity);
		void SetAllComponentFields(MonoProjectComponentInstance& monoProjectComp, MonoScriptComponent& monoScriptComp);

	private:
		friend class Scripting;

		Application& _app;
		Scene& _scene;

		float _timeSinceStart = 0.f;

		std::unordered_map<AssetHandle, MonoAsset> _monoAssets;

		std::vector<std::function<void(Entity, Ref<MonoEntity>)>> _componentSyncFuncs;
		std::unordered_map<UUID, Ref<MonoEntity>> _monoEntities;
		std::vector<UUID> _monoEntitiesToDestroy;
	};
}