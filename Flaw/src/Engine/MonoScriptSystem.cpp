#include "pch.h"
#include "MonoScriptSystem.h"
#include "Application.h"
#include "Scene.h"
#include "Components.h"
#include "Time/Time.h"
#include "AssetManager.h"
#include "PhysicsSystem.h"

namespace flaw {
	MonoEngineComponentInstance::MonoEngineComponentInstance(const UUID& uuid, const char* name) 
		: _scriptObject(Scripting::GetMonoClass(name))
	{
		_scriptObject.Instantiate(&uuid);
	}

	MonoProjectComponentInstance::MonoProjectComponentInstance(const UUID& uuid, const char* name) 
		: _scriptObject(Scripting::GetMonoClass(name))
	{
		_scriptObject.Instantiate(&uuid);
		CreatePublicFieldsInObjectRecursive(_scriptObject.GetView());

		auto monoClass = _scriptObject.GetClass();
		_createMethod = monoClass.GetMethodRecurcive("OnCreate", 0);
		_startMethod = monoClass.GetMethodRecurcive("OnStart", 0);
		_updateMethod = monoClass.GetMethodRecurcive("OnUpdate", 0);
		_destroyMethod = monoClass.GetMethodRecurcive("OnDestroy", 0);
		_onCollisionEnterMethod = monoClass.GetMethodRecurcive("OnCollisionEnter", 1);
		_onCollisionStayMethod = monoClass.GetMethodRecurcive("OnCollisionStay", 1);
		_onCollisionExitMethod = monoClass.GetMethodRecurcive("OnCollisionExit", 1);
		_onTriggerEnterMethod = monoClass.GetMethodRecurcive("OnTriggerEnter", 1);
		_onTriggerStayMethod = monoClass.GetMethodRecurcive("OnTriggerStay", 1);
		_onTriggerExitMethod = monoClass.GetMethodRecurcive("OnTriggerExit", 1);
	}

	void MonoProjectComponentInstance::CreatePublicFieldsInObjectRecursive(MonoScriptObjectView& objView) {
		objView.GetClass().EachFields([&objView](std::string_view fieldName, MonoScriptClassField& field) {
			if (field.IsClass()) {
				MonoObject* objPtr = field.GetValue<MonoObject*>(objView);
				if (!objPtr) {
					MonoScriptObject fieldMonoObj(field.GetClass());
					fieldMonoObj.Instantiate();
					field.SetValue(objView, fieldMonoObj.GetMonoObject());
					CreatePublicFieldsInObjectRecursive(fieldMonoObj.GetView());
				}
				else {
					MonoScriptObjectView fieldMonoObjView(field.GetClass(), objPtr);
					CreatePublicFieldsInObjectRecursive(fieldMonoObjView);
				}
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

	void MonoProjectComponentInstance::CallOnCollisionEnter(MonoScriptObject& collisionInfo) const {
		if (!_onCollisionEnterMethod) {
			return;
		}
		_scriptObject.CallMethod(_onCollisionEnterMethod, collisionInfo.GetMonoObject());
	}

	void MonoProjectComponentInstance::CallOnCollisionStay(MonoScriptObject& collisionInfo) const {
		if (!_onCollisionStayMethod) {
			return;
		}
		_scriptObject.CallMethod(_onCollisionStayMethod, collisionInfo.GetMonoObject());
	}

	void MonoProjectComponentInstance::CallOnCollisionExit(MonoScriptObject& collisionInfo) const {
		if (!_onCollisionExitMethod) {
			return;
		}
		_scriptObject.CallMethod(_onCollisionExitMethod, collisionInfo.GetMonoObject());
	}

	MonoEntity::MonoEntity(const UUID& uuid)
		: _uuid(uuid)
		, _scriptObject(Scripting::GetMonoClass(Scripting::MonoEntityClassName))
	{
		_scriptObject.Instantiate(&uuid);
	}

	MonoComponentInstance* MonoEntity::AddComponent(const char* name) {
		if (!Scripting::HasMonoClass(name)) {
			return nullptr;
		}

		auto& monoClass = Scripting::GetMonoClass(name);
		if (Scripting::IsMonoProjectComponent(monoClass)) {
			_projectComponents.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(_uuid, name));
			return &_projectComponents[name];
		}
		else {
			_engineComponents.emplace(std::piecewise_construct, std::forward_as_tuple(name), std::forward_as_tuple(_uuid, name));
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

	MonoAsset::MonoAsset(const AssetHandle& handle, const Ref<Asset>& asset) 
		: _scriptObject(Scripting::GetMonoAssetClass(asset->GetAssetType()))
	{
		_scriptObject.Instantiate(&handle);
	}

	MonoScriptSystem::MonoScriptSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
		, _timeSinceStart(0.f)
	{
	}

	void MonoScriptSystem::SetAllComponentFields(MonoProjectComponentInstance& monoProjectComp, MonoScriptComponent& monoScriptComp) {
		auto& monoScriptObj = monoProjectComp.GetScriptObject();
		auto monoScriptClass = monoScriptObj.GetClass();
		for (const auto& fieldInfo : monoScriptComp.fields) {
			auto field = monoScriptClass.GetFieldRecursive(fieldInfo.fieldName.c_str());
			if (!field) {
				continue;
			}

			auto fieldClass = field.GetClass();
			if (fieldClass.GetTypeName() != fieldInfo.fieldType) {
				continue;
			}

			if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Float)) {
				float value = fieldInfo.As<float>();
				field.SetValue(monoScriptObj, &value);
			}
			else if (fieldClass == Scripting::GetMonoSystemClass(MonoSystemType::Int32)) {
				int32_t value = fieldInfo.As<int32_t>();
				field.SetValue(monoScriptObj, &value);
			}
			else if (Scripting::IsMonoComponent(fieldClass)) {
				UUID uuid = fieldInfo.As<UUID>();
				auto it = _monoEntities.find(uuid);
				if (it == _monoEntities.end()) {
					continue;
				}

				auto* targetComp = it->second->GetComponent(fieldInfo.fieldType.c_str());
				if (!targetComp) {
					continue;
				}

				field.SetValue(monoScriptObj, targetComp->GetScriptObject().GetMonoObject());
			}
			else if (Scripting::IsMonoAsset(fieldClass)) {
				AssetHandle assetHandle = fieldInfo.As<AssetHandle>();
				auto it = _monoAssets.find(assetHandle);
				if (it == _monoAssets.end()) {
					continue;
				}

				field.SetValue(monoScriptObj, it->second.GetScriptObject().GetMonoObject());
			}
			else if (fieldClass == Scripting::GetMonoClass(Scripting::MonoEntityClassName)) {
				UUID uuid = fieldInfo.As<UUID>();
				auto it = _monoEntities.find(uuid);
				if (it == _monoEntities.end()) {
					continue;
				}	

				field.SetValue(monoScriptObj, it->second->GetScriptObject().GetMonoObject());
			}
		}
	}

	void MonoScriptSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		_monoEntities[enttComp.uuid] = CreateRef<MonoEntity>(enttComp.uuid);
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

		_monoEntitiesToDestroy.push_back(monoEntt);
		_monoEntities.erase(it);
	}

	void MonoScriptSystem::RegisterMonoComponentInRuntime(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		auto it = _monoEntities.find(enttComp.uuid);
		if (it == _monoEntities.end()) {
			return;
		}

		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		auto monoEntt = it->second;
		auto* prjComp = static_cast<MonoProjectComponentInstance*>(monoEntt->AddComponent(monoScriptComp.name.c_str()));
		if (!prjComp) {
			return;
		}

		SetAllComponentFields(*prjComp, monoScriptComp);

		prjComp->CallOnCreate();
		prjComp->CallOnStart();
	}

	void MonoScriptSystem::Start() {
		auto& registry = _scene.GetRegistry();
		auto& physicsSys = _scene.GetPhysicsSystem();

		Scripting::SetActiveMonoScriptSystem(this);

		physicsSys.RegisterOnCollisionEnterHandler(PID(this), std::bind(&MonoScriptSystem::HandleOnCollisionEnter, this, std::placeholders::_1));
		physicsSys.RegisterOnCollisionStayHandler(PID(this), std::bind(&MonoScriptSystem::HandleOnCollisionStay, this, std::placeholders::_1));
		physicsSys.RegisterOnCollisionExitHandler(PID(this), std::bind(&MonoScriptSystem::HandleOnCollisionExit, this, std::placeholders::_1));

		registry.on_construct<EntityComponent>().connect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().connect<&MonoScriptSystem::UnregisterEntity>(*this);
		registry.on_construct<TransformComponent>().connect<&MonoScriptSystem::RegisterComponent<TransformComponent>>(*this);
		registry.on_construct<CameraComponent>().connect<&MonoScriptSystem::RegisterComponent<CameraComponent>>(*this);
		registry.on_construct<BoxColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<MeshColliderComponent>>(*this);
		registry.on_construct<MonoScriptComponent>().connect<&MonoScriptSystem::RegisterMonoComponentInRuntime>(*this);

		_timeSinceStart = 0.f;

		// NOTE: asset MonoObjects를 초기화합니다.
		_monoAssets.clear();
		AssetManager::EachAssets([this](const AssetHandle& handle, const Ref<Asset>& asset) { _monoAssets.emplace(handle, MonoAsset(handle, asset)); });

		// NOTE: monoEntities를 초기화합니다.
		_monoEntities.clear();
		for (auto&& [entity, enttComp] : registry.view<EntityComponent>().each()) {
			RegisterEntity(registry, entity);
			RegisterComponent<TransformComponent>(registry, entity);
			RegisterComponent<CameraComponent>(registry, entity);
			RegisterComponent<BoxColliderComponent>(registry, entity);
			RegisterComponent<SphereColliderComponent>(registry, entity);
			RegisterComponent<MeshColliderComponent>(registry, entity);
			RegisterComponent<AnimatorComponent>(registry, entity);
			RegisterComponent<MonoScriptComponent>(registry, entity);
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

	const char* MonoScriptSystem::GetMonoColliderClassName(PhysicsShapeType shapeType) const {
		switch (shapeType) {
			case PhysicsShapeType::Box:
				return Scripting::MonoBoxColliderComponentClassName;
			case PhysicsShapeType::Sphere:
				return Scripting::MonoSphereColliderComponentClassName;
			case PhysicsShapeType::Mesh:
				return Scripting::MonoMeshColliderComponentClassName;
			default:
				throw std::runtime_error("Unsupported PhysicsShapeType for MonoScriptSystem");
		}
	}

	std::vector<MonoScriptObject> MonoScriptSystem::CreateContactPointObjects(const std::vector<ContactPoint>& contactPoints) const {
		auto& contactPointClass = Scripting::GetMonoClass(Scripting::MonoContactPointClassName);

		std::vector<MonoScriptObject> contactPointObjects;
		for (const auto& contactPoint : contactPoints) {
			MonoScriptObject contactPointObj(contactPointClass);
			contactPointObj.Instantiate(&contactPoint.position, &contactPoint.normal, &contactPoint.impulse);
			contactPointObjects.emplace_back(std::move(contactPointObj));
		}

		return contactPointObjects;
	}

	MonoScriptObject MonoScriptSystem::CreateCollisionInfoObject(MonoScriptObject& colliderA, MonoScriptObject& colliderB, MonoScriptArray& contactArray) const {
		auto& collisionInfoClass = Scripting::GetMonoClass(Scripting::MonoCollisionInfoClassName);

		MonoScriptObject collisionInfoObj(collisionInfoClass);
		collisionInfoObj.Instantiate(colliderA.GetMonoObject(), colliderB.GetMonoObject(), contactArray.GetMonoArray());

		return collisionInfoObj;
	}

	void MonoScriptSystem::HandleOnCollisionEnter(const CollisionInfo& collisionInfo) const {
		if (!collisionInfo.entity0 || !collisionInfo.entity1) {
			return;
		}

		auto& monoEnttA = GetMonoEntity(collisionInfo.entity0.GetUUID());
		auto& monoEnttB = GetMonoEntity(collisionInfo.entity1.GetUUID());

		auto* monoEnttAColliderComp = monoEnttA->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto* monoEnttBColliderComp = monoEnttB->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType1));

		std::vector<MonoScriptObject> contactPointObjects = CreateContactPointObjects(collisionInfo.contactPoints);
		MonoScriptArray contactPointArray(contactPointObjects.data(), contactPointObjects.size());

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttA->GetProjectComponents()) {
			comp.CallOnCollisionEnter(collisionInfoObj0);
		}

		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttB->GetProjectComponents()) {
			comp.CallOnCollisionEnter(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::HandleOnCollisionStay(const CollisionInfo& collisionInfo) const {
		if (!collisionInfo.entity0 || !collisionInfo.entity1) {
			return;
		}

		auto& monoEnttA = GetMonoEntity(collisionInfo.entity0.GetUUID());
		auto& monoEnttB = GetMonoEntity(collisionInfo.entity1.GetUUID());
		
		auto* monoEnttAColliderComp = monoEnttA->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto* monoEnttBColliderComp = monoEnttB->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType1));

		std::vector<MonoScriptObject> contactPointObjects = CreateContactPointObjects(collisionInfo.contactPoints);
		MonoScriptArray contactPointArray(contactPointObjects.data(), contactPointObjects.size());

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttA->GetProjectComponents()) {
			comp.CallOnCollisionStay(collisionInfoObj0);
		}
		
		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttB->GetProjectComponents()) {
			comp.CallOnCollisionStay(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::HandleOnCollisionExit(const CollisionInfo& collisionInfo) const {
		if (!collisionInfo.entity0 || !collisionInfo.entity1) {
			return;
		}

		auto& monoEnttA = GetMonoEntity(collisionInfo.entity0.GetUUID());
		auto& monoEnttB = GetMonoEntity(collisionInfo.entity1.GetUUID());
		
		auto* monoEnttAColliderComp = monoEnttA->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto* monoEnttBColliderComp = monoEnttB->GetComponent(GetMonoColliderClassName(collisionInfo.shapeType1));
		
		MonoScriptArray contactPointArray(Scripting::GetMonoClass(Scripting::MonoContactPointClassName));
		contactPointArray.Instantiate(0);

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttA->GetProjectComponents()) {
			comp.CallOnCollisionExit(collisionInfoObj0);
		}

		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& [name, comp] : monoEnttB->GetProjectComponents()) {
			comp.CallOnCollisionExit(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::Update() {
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

		_monoEntitiesToDestroy.clear();

		_timeSinceStart += Time::DeltaTime();
	}

	void MonoScriptSystem::End() {
		auto& registry = _scene.GetRegistry();
		auto& physicsSys = _scene.GetPhysicsSystem();

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

		registry.on_construct<EntityComponent>().disconnect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().disconnect<&MonoScriptSystem::UnregisterEntity>(*this);
		registry.on_construct<TransformComponent>().disconnect<&MonoScriptSystem::RegisterComponent<TransformComponent>>(*this);
		registry.on_construct<CameraComponent>().disconnect<&MonoScriptSystem::RegisterComponent<CameraComponent>>(*this);
		registry.on_construct<BoxColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<MeshColliderComponent>>(*this);
		registry.on_construct<MonoScriptComponent>().disconnect<&MonoScriptSystem::RegisterMonoComponentInRuntime>(*this);

		physicsSys.UnregisterOnCollisionEnterHandler(PID(this));
		physicsSys.UnregisterOnCollisionStayHandler(PID(this));
		physicsSys.UnregisterOnCollisionExitHandler(PID(this));

		Scripting::SetActiveMonoScriptSystem(nullptr);
	}

	bool MonoScriptSystem::IsMonoEntityExists(const UUID& uuid) const {
		return _monoEntities.find(uuid) != _monoEntities.end();
	}

	Ref<MonoEntity> MonoScriptSystem::GetMonoEntity(const UUID& uuid) const {
		auto it = _monoEntities.find(uuid);
		if (it != _monoEntities.end()) {
			return it->second;
		}

		throw std::runtime_error("Mono script instance not found for UUID");
	}
}