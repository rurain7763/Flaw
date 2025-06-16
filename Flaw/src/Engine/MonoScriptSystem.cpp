#include "pch.h"
#include "MonoScriptSystem.h"
#include "Application.h"
#include "Scene.h"
#include "Components.h"
#include "Time/Time.h"
#include "AssetManager.h"

namespace flaw {
	MonoEngineComponentInstance::MonoEngineComponentInstance(const UUID& uuid, const char* name) {
		auto& monoClass = Scripting::GetMonoClass(name);

		_scriptObject = MonoScriptObject(&monoClass);

		UUID copy = uuid;
		void* args[] = { &copy };
		_scriptObject.Instantiate(args, 1);
	}

	MonoProjectComponentInstance::MonoProjectComponentInstance(const UUID& uuid, const char* name) {
		auto& monoClass = Scripting::GetMonoClass(name);

		_scriptObject = MonoScriptObject(&monoClass);

		UUID copy = uuid;
		void* args[] = { &copy };
		_scriptObject.Instantiate(args, 1);
		CreatePublicFieldsInObjectRecursive(_scriptObject);

		_createMethod = monoClass.GetMethodRecurcive("OnCreate", 0);
		_startMethod = monoClass.GetMethodRecurcive("OnStart", 0);
		_updateMethod = monoClass.GetMethodRecurcive("OnUpdate", 0);
		_destroyMethod = monoClass.GetMethodRecurcive("OnDestroy", 0);
	}

	void MonoProjectComponentInstance::CreatePublicFieldsInObjectRecursive(MonoScriptObject& object) {
		object.GetClass().EachFields([&object](std::string_view fieldName, MonoScriptClassField& field) {
			if (field.IsClass()) {
				MonoScriptObject fieldMonoObj(object.GetMonoDomain(), field.GetMonoClass(), field.GetValue<MonoObject*>(&object));
				fieldMonoObj.Instantiate();
				field.SetValue(&object, fieldMonoObj.GetMonoObject());

				CreatePublicFieldsInObjectRecursive(fieldMonoObj);
			}
		});
	}

	void MonoProjectComponentInstance::CallOnCreate() const {
		if (_createMethod) {
			_scriptObject.CallMethod(_createMethod);
		}
	}

	void MonoProjectComponentInstance::CallOnStart() const {
		if (_startMethod) {
			_scriptObject.CallMethod(_startMethod);
		}
	}

	void MonoProjectComponentInstance::CallOnUpdate() const {
		if (_updateMethod) {
			_scriptObject.CallMethod(_updateMethod);
		}
	}

	void MonoProjectComponentInstance::CallOnDestroy() const {
		if (_destroyMethod) {
			_scriptObject.CallMethod(_destroyMethod);
		}
	}

	MonoEntity::MonoEntity(const UUID& uuid)
		: _uuid(uuid)
		, _scriptObject(&Scripting::GetMonoClass(Scripting::EntityClassName))
	{
		void* args[] = { (void*)&_uuid };
		_scriptObject.Instantiate(args, 1);
	}

	MonoComponentInstance* MonoEntity::AddComponent(const char* name) {
		if (!Scripting::HasMonoClass(name)) {
			return nullptr;
		}

		auto& monoClass = Scripting::GetMonoClass(name);
		if (Scripting::IsMonoProjectComponent(monoClass)) {
			_projectComponents.emplace(name, MonoProjectComponentInstance(_uuid, name));
			return &_projectComponents[name];
		}
		else {
			_engineComponents.emplace(name, MonoEngineComponentInstance(_uuid, name));
			return &_engineComponents[name];
		}
	}

	void MonoEntity::RemoveComponent(const char* name) {
		_engineComponents.erase(name);
		_projectComponents.erase(name);
	}

	MonoComponentInstance* MonoEntity::GetComponent(const char* name) {
		auto engineIt = _engineComponents.find(name);
		if (engineIt != _engineComponents.end()) {
			return &engineIt->second;
		}

		auto projectIt = _projectComponents.find(name);
		if (projectIt != _projectComponents.end()) {
			return &projectIt->second;
		}

		return nullptr;
	}

	MonoProjectComponentInstance* MonoEntity::GetProjectComponent(const char* name) {
		auto it = _projectComponents.find(name);
		if (it != _projectComponents.end()) {
			return &it->second;
		}
		return nullptr;
	}

	bool MonoEntity::HasComponent(const char* name) const {
		return (_engineComponents.find(name) != _engineComponents.end()) || (_projectComponents.find(name) != _projectComponents.end());
	}

	MonoAsset::MonoAsset(const AssetHandle& handle, const Ref<Asset>& asset) {
		MonoScriptClass monoClass;
		if (asset->GetAssetType() == AssetType::Prefab) {
			monoClass = Scripting::GetMonoAssetClass(AssetType::Prefab);
		}
		else {
			return;
		}

		_scriptObject = MonoScriptObject(&monoClass);

		void* args[] = { (void*)&handle };
		_scriptObject.Instantiate(args, 1);
	}

	MonoScriptSystem::MonoScriptSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
		, _timeSinceStart(0.f)
	{
		RegisterComponentSyncFunc<TransformComponent>();
		RegisterComponentSyncFunc<CameraComponent>();
	}

	Ref<MonoEntity> MonoScriptSystem::CreateMonoEntityByEntity(const Entity& entity) {
		auto monoEntt = CreateRef<MonoEntity>(entity.GetUUID());
		auto& monoScriptComp = entity.GetComponent<MonoScriptComponent>();

		for (const auto& syncFunc : _componentSyncFuncs) {
			syncFunc(entity, monoEntt);
		}

		monoEntt->AddComponent(monoScriptComp.name.c_str());

		return monoEntt;
	}

	void MonoScriptSystem::SetAllComponentFields(MonoProjectComponentInstance& monoProjectComp, MonoScriptComponent& monoScriptComp) {
		auto& monoScriptObj = monoProjectComp.GetScriptObject();
		auto monoScriptClass = monoScriptObj.GetClass();
		for (const auto& fieldInfo : monoScriptComp.fields) {
			auto field = monoScriptClass.GetFieldRecursive(fieldInfo.fieldName.c_str());
			if (!field || field.GetTypeName() != fieldInfo.fieldType) {
				continue;
			}

			MonoScriptClass filedClass(monoScriptObj.GetMonoDomain(), field.GetMonoClass());
			if (field.GetMonoClass() == Scripting::GetMonoSystemClass(MonoSystemType::Float).GetMonoClass()) {
				float value = fieldInfo.As<float>();
				field.SetValue(&monoScriptObj, &value);
			}
			else if (field.GetMonoClass() == Scripting::GetMonoSystemClass(MonoSystemType::Int32).GetMonoClass()) {
				int32_t value = fieldInfo.As<int32_t>();
				field.SetValue(&monoScriptObj, &value);
			}
			else if (Scripting::IsMonoComponent(filedClass)) {
				UUID uuid = fieldInfo.As<UUID>();
				auto it = _monoEntities.find(uuid);
				if (it == _monoEntities.end()) {
					continue;
				}

				auto* targetComp = it->second->GetComponent(fieldInfo.fieldType.c_str());
				if (!targetComp) {
					continue;
				}

				field.SetValue(&monoScriptObj, targetComp->GetScriptObject().GetMonoObject());
			}
			else if (Scripting::IsMonoAsset(filedClass)) {
				AssetHandle assetHandle = fieldInfo.As<AssetHandle>();
				auto it = _monoAssets.find(assetHandle);
				if (it == _monoAssets.end()) {
					continue;
				}

				field.SetValue(&monoScriptObj, it->second.GetScriptObject().GetMonoObject());
			}
			else if (filedClass == Scripting::GetMonoClass(Scripting::EntityClassName)) {
				UUID uuid = fieldInfo.As<UUID>();
				auto it = _monoEntities.find(uuid);
				if (it == _monoEntities.end()) {
					continue;
				}	

				field.SetValue(&monoScriptObj, it->second->GetScriptObject().GetMonoObject());
			}
		}
	}

	void MonoScriptSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		auto newMonoEntt = CreateMonoEntityByEntity(Entity(entity, &_scene));
		_monoEntities[enttComp.uuid] = newMonoEntt;

		auto* prjComp = newMonoEntt->GetProjectComponent(monoScriptComp.name.c_str());
		SetAllComponentFields(*prjComp, monoScriptComp);

		prjComp->CallOnCreate();
		prjComp->CallOnStart();
	}

	void MonoScriptSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		auto it = _monoEntities.find(enttComp.uuid);
		if (it == _monoEntities.end()) {
			return;
		}

		auto monoEntt = it->second;
		for (const auto& [name, comp] : monoEntt->GetProjectComponents()) {
			comp.CallOnDestroy();
		}

		_monoEntitiesToDestroy.push_back(enttComp.uuid);		
	}

	void MonoScriptSystem::Start() {
		auto& registry = _scene.GetRegistry();

		Scripting::SetActiveMonoScriptSystem(this);

		registry.on_construct<MonoScriptComponent>().connect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().connect<&MonoScriptSystem::UnregisterEntity>(*this);

		_timeSinceStart = 0.f;

		// NOTE: asset MonoObjects를 초기화합니다.
		_monoAssets.clear();
		AssetManager::EachAssets([this](const AssetHandle& handle, const Ref<Asset>& asset) { _monoAssets.emplace(handle, MonoAsset(handle, asset)); });

		// NOTE: monoEntities를 초기화합니다.
		_monoEntities.clear();
		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			_monoEntities[enttComp.uuid] = CreateMonoEntityByEntity(Entity(entity, &_scene));
		}

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto monoEntt = it->second;
			auto* prjComp = monoEntt->GetProjectComponent(monoScriptComp.name.c_str());
			if (!prjComp) {
				continue;
			}

			SetAllComponentFields(*prjComp, monoScriptComp);
		}

		// NOTE: MonoEntity의 컴포넌트들의 OnCreate를 호출합니다.
		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto monoEntt = it->second;
			for (const auto& [name, comp] : monoEntt->GetProjectComponents()) {
				comp.CallOnCreate();
			}
		}

		// NOTE: MonoEntity의 컴포넌트들의 OnStart를 호출합니다.
		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto monoEntt = it->second;
			for (const auto& [name, comp] : monoEntt->GetProjectComponents()) {
				comp.CallOnStart();
			}
		}
	}

	void MonoScriptSystem::Update() {
		for (auto& uuid : _monoEntitiesToDestroy) {
			_monoEntities.erase(uuid);
		}
		_monoEntitiesToDestroy.clear();

		for (auto&& [entity, enttComp, monoScriptComp] : _scene.GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto& monoEntt = it->second;
			for (const auto& [name, comp] : monoEntt->GetProjectComponents()) {
				comp.CallOnUpdate();
			}
		}

		_timeSinceStart += Time::DeltaTime();
	}

	void MonoScriptSystem::End() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto& monoEntt = it->second;
			for (const auto& [name, comp] : monoEntt->GetProjectComponents()) {
				comp.CallOnDestroy();
			}
		}

		registry.on_construct<MonoScriptComponent>().disconnect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().disconnect<&MonoScriptSystem::UnregisterEntity>(*this);

		Scripting::SetActiveMonoScriptSystem(nullptr);
	}

	bool MonoScriptSystem::IsMonoEntityExists(const UUID& uuid) const {
		return _monoEntities.find(uuid) != _monoEntities.end();
	}

	Ref<MonoEntity> MonoScriptSystem::GetMonoEntity(const UUID& uuid) {
		auto it = _monoEntities.find(uuid);
		if (it != _monoEntities.end()) {
			return it->second;
		}

		throw std::runtime_error("Mono script instance not found for UUID");
	}
}