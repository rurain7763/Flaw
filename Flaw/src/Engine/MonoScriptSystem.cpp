#include "pch.h"
#include "MonoScriptSystem.h"
#include "Application.h"
#include "Scene.h"
#include "Components.h"
#include "Time/Time.h"
#include "AssetManager.h"
#include "PhysicsSystem.h"

namespace flaw {
	MonoEngineComponentRuntime::MonoEngineComponentRuntime(const UUID& uuid, const char* name) 
		: _scriptObject(Scripting::GetMonoClass(name))
	{
		_scriptObject.Instantiate(&uuid);
	}

	MonoProjectComponentRuntime::MonoProjectComponentRuntime(const UUID& uuid, const char* name) 
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

	void MonoProjectComponentRuntime::CreatePublicFieldsInObjectRecursive(MonoScriptObjectView& objView) {
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

	void MonoProjectComponentRuntime::CallOnCreate() const {
		if (_createMethod) {
			_scriptObject.CallMethod(_createMethod);
		}
	}

	void MonoProjectComponentRuntime::CallOnStart() const {
		if (_startMethod) {
			_scriptObject.CallMethod(_startMethod);
		}
	}

	void MonoProjectComponentRuntime::CallOnUpdate() const {
		if (_updateMethod) {
			_scriptObject.CallMethod(_updateMethod);
		}
	}

	void MonoProjectComponentRuntime::CallOnDestroy() const {
		if (_destroyMethod) {
			_scriptObject.CallMethod(_destroyMethod);
		}
	}

	void MonoProjectComponentRuntime::CallOnCollisionEnter(MonoScriptObject& collisionInfo) const {
		if (!_onCollisionEnterMethod) {
			return;
		}
		_scriptObject.CallMethod(_onCollisionEnterMethod, collisionInfo.GetMonoObject());
	}

	void MonoProjectComponentRuntime::CallOnCollisionStay(MonoScriptObject& collisionInfo) const {
		if (!_onCollisionStayMethod) {
			return;
		}
		_scriptObject.CallMethod(_onCollisionStayMethod, collisionInfo.GetMonoObject());
	}

	void MonoProjectComponentRuntime::CallOnCollisionExit(MonoScriptObject& collisionInfo) const {
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

	Ref<MonoComponentRuntime> MonoEntity::AddComponent(const char* name) {
		if (!Scripting::HasMonoClass(name)) {
			return nullptr;
		}

		auto& monoClass = Scripting::GetMonoClass(name);

		Ref<MonoComponentRuntime> monoComp;

		if (Scripting::IsMonoProjectComponent(monoClass)) {
			auto prjComp = CreateRef<MonoProjectComponentRuntime>(_uuid, name);
			_projectComponents.insert(prjComp);
			monoComp = prjComp;
		}
		else {
			monoComp = CreateRef<MonoEngineComponentRuntime>(_uuid, name);
		}

		_components[name] = monoComp;

		return monoComp;
	}

	void MonoEntity::RemoveComponent(const char* name) {
		auto it = _components.find(name);
		if (it == _components.end()) {
			return;
		}

		if (auto prjComp = std::dynamic_pointer_cast<MonoProjectComponentRuntime>(it->second)) {
			_projectComponents.erase(prjComp);
		}

		_components.erase(name);
	}

	bool MonoEntity::HasComponent(const char* name) const {
		return _components.find(name) != _components.end();
	}

	Ref<MonoComponentRuntime> MonoEntity::GetComponent(const char* name) {
		auto it = _components.find(name);
		if (it != _components.end()) {
			return it->second;
		}
		return nullptr;
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

	void MonoScriptSystem::SetAllComponentFields(MonoProjectComponentRuntime& monoProjectComp, MonoScriptComponent& monoScriptComp) {
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

				auto targetComp = it->second->GetComponent(fieldInfo.fieldType.c_str());
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

		_monoEntitiesToDestroy.push_back(it->second);
		_monoEntities.erase(it);
	}

	void MonoScriptSystem::RegisterMonoComponent(entt::registry& registry, entt::entity entity) {
		if (!registry.any_of<MonoScriptComponent>(entity)) {
			return;
		}

		auto& enttComp = registry.get<EntityComponent>(entity);
		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		auto it = _monoEntities.find(enttComp.uuid);
		if (it == _monoEntities.end()) {
			return;
		}

		auto monoEntt = it->second;

		monoEntt->AddComponent(monoScriptComp.name.c_str());
	}

	void MonoScriptSystem::RegisterMonoComponentInRuntime(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		auto it = _monoEntities.find(enttComp.uuid);
		if (it == _monoEntities.end()) {
			return;
		}

		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		auto monoEntt = it->second;
		auto prjComp = std::static_pointer_cast<MonoProjectComponentRuntime>(monoEntt->AddComponent(monoScriptComp.name.c_str()));
		if (!prjComp) {
			return;
		}

		if (_defferedInitComponents) {
			_deferredInitComponents.push_back({ monoEntt, prjComp });
			return;
		}

		SetAllComponentFields(*prjComp, monoScriptComp);

		prjComp->CallOnCreate();
		prjComp->CallOnStart();
	}

	void MonoScriptSystem::UnregisterMonoComponent(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		auto monoEntt = GetMonoEntity(enttComp.uuid);
		if (!monoEntt) {
			return;
		}

		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		auto prjComp = monoEntt->GetComponent<MonoProjectComponentRuntime>(monoScriptComp.name.c_str());
		prjComp->CallOnDestroy();

		monoEntt->RemoveComponent(monoScriptComp.name.c_str());
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
		registry.on_destroy<TransformComponent>().connect<&MonoScriptSystem::UnregisterComponent<TransformComponent>>(*this);
		registry.on_construct<CameraComponent>().connect<&MonoScriptSystem::RegisterComponent<CameraComponent>>(*this);
		registry.on_destroy<CameraComponent>().connect<&MonoScriptSystem::UnregisterComponent<CameraComponent>>(*this);
		registry.on_construct<BoxColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().connect<&MonoScriptSystem::UnregisterComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().connect<&MonoScriptSystem::UnregisterComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().connect<&MonoScriptSystem::RegisterComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().connect<&MonoScriptSystem::UnregisterComponent<MeshColliderComponent>>(*this);
		registry.on_construct<AnimatorComponent>().connect<&MonoScriptSystem::RegisterComponent<AnimatorComponent>>(*this);
		registry.on_destroy<AnimatorComponent>().connect<&MonoScriptSystem::UnregisterComponent<AnimatorComponent>>(*this);
		registry.on_construct<SkeletalMeshComponent>().connect<&MonoScriptSystem::RegisterComponent<SkeletalMeshComponent>>(*this);
		registry.on_destroy<SkeletalMeshComponent>().connect<&MonoScriptSystem::UnregisterComponent<SkeletalMeshComponent>>(*this);
		registry.on_construct<MonoScriptComponent>().connect<&MonoScriptSystem::RegisterMonoComponentInRuntime>(*this);
		registry.on_destroy<MonoScriptComponent>().connect<&MonoScriptSystem::UnregisterMonoComponent>(*this);

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
			RegisterComponent<SkeletalMeshComponent>(registry, entity);
			RegisterMonoComponent(registry, entity);
		}

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto monoEntt = it->second;
			auto prjComp = monoEntt->GetComponent<MonoProjectComponentRuntime>(monoScriptComp.name.c_str());
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
			for (const auto& comp : monoEntt->GetProjectComponents()) {
				comp->CallOnCreate();
			}
		}

		// NOTE: MonoEntity의 컴포넌트들의 OnStart를 호출합니다.
		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto monoEntt = it->second;
			for (const auto& comp : monoEntt->GetProjectComponents()) {
				comp->CallOnStart();
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

		auto monoEnttAColliderComp = monoEnttA->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto monoEnttBColliderComp = monoEnttB->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType1));

		std::vector<MonoScriptObject> contactPointObjects = CreateContactPointObjects(collisionInfo.contactPoints);
		MonoScriptArray contactPointArray(contactPointObjects.data(), contactPointObjects.size());

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttA->GetProjectComponents()) {
			comp->CallOnCollisionEnter(collisionInfoObj0);
		}

		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttB->GetProjectComponents()) {
			comp->CallOnCollisionEnter(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::HandleOnCollisionStay(const CollisionInfo& collisionInfo) const {
		if (!collisionInfo.entity0 || !collisionInfo.entity1) {
			return;
		}

		auto& monoEnttA = GetMonoEntity(collisionInfo.entity0.GetUUID());
		auto& monoEnttB = GetMonoEntity(collisionInfo.entity1.GetUUID());

		auto monoEnttAColliderComp = monoEnttA->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto monoEnttBColliderComp = monoEnttB->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType1));

		std::vector<MonoScriptObject> contactPointObjects = CreateContactPointObjects(collisionInfo.contactPoints);
		MonoScriptArray contactPointArray(contactPointObjects.data(), contactPointObjects.size());

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttA->GetProjectComponents()) {
			comp->CallOnCollisionStay(collisionInfoObj0);
		}

		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttB->GetProjectComponents()) {
			comp->CallOnCollisionStay(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::HandleOnCollisionExit(const CollisionInfo& collisionInfo) const {
		if (!collisionInfo.entity0 || !collisionInfo.entity1) {
			return;
		}

		auto& monoEnttA = GetMonoEntity(collisionInfo.entity0.GetUUID());
		auto& monoEnttB = GetMonoEntity(collisionInfo.entity1.GetUUID());
		
		auto monoEnttAColliderComp = monoEnttA->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType0));
		auto monoEnttBColliderComp = monoEnttB->GetComponent<MonoEngineComponentRuntime>(GetMonoColliderClassName(collisionInfo.shapeType1));

		MonoScriptArray contactPointArray(Scripting::GetMonoClass(Scripting::MonoContactPointClassName));
		contactPointArray.Instantiate(0);

		auto collisionInfoObj0 = CreateCollisionInfoObject(monoEnttAColliderComp->GetScriptObject(), monoEnttBColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttA->GetProjectComponents()) {
			comp->CallOnCollisionExit(collisionInfoObj0);
		}

		auto collisionInfoObj1 = CreateCollisionInfoObject(monoEnttBColliderComp->GetScriptObject(), monoEnttAColliderComp->GetScriptObject(), contactPointArray);
		for (const auto& comp : monoEnttB->GetProjectComponents()) {
			comp->CallOnCollisionExit(collisionInfoObj1);
		}
	}

	void MonoScriptSystem::Update() {
		for (auto&& [entity, enttComp, monoScriptComp] : _scene.GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoEntities.find(enttComp.uuid);
			if (it == _monoEntities.end()) {
				continue;
			}

			auto& monoEntt = it->second;
			for (const auto& comp : monoEntt->GetProjectComponents()) {
				comp->CallOnUpdate();
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
			for (const auto& comp : monoEntt->GetProjectComponents()) {
				comp->CallOnDestroy();
			}
		}

		registry.on_construct<EntityComponent>().disconnect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().disconnect<&MonoScriptSystem::UnregisterEntity>(*this);
		registry.on_construct<TransformComponent>().disconnect<&MonoScriptSystem::RegisterComponent<TransformComponent>>(*this);
		registry.on_destroy<TransformComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<TransformComponent>>(*this);
		registry.on_construct<CameraComponent>().disconnect<&MonoScriptSystem::RegisterComponent<CameraComponent>>(*this);
		registry.on_destroy<CameraComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<CameraComponent>>(*this);
		registry.on_construct<BoxColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<BoxColliderComponent>>(*this);
		registry.on_destroy<BoxColliderComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<BoxColliderComponent>>(*this);
		registry.on_construct<SphereColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<SphereColliderComponent>>(*this);
		registry.on_destroy<SphereColliderComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<SphereColliderComponent>>(*this);
		registry.on_construct<MeshColliderComponent>().disconnect<&MonoScriptSystem::RegisterComponent<MeshColliderComponent>>(*this);
		registry.on_destroy<MeshColliderComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<MeshColliderComponent>>(*this);
		registry.on_construct<AnimatorComponent>().disconnect<&MonoScriptSystem::RegisterComponent<AnimatorComponent>>(*this);
		registry.on_destroy<AnimatorComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<AnimatorComponent>>(*this);
		registry.on_construct<SkeletalMeshComponent>().disconnect<&MonoScriptSystem::RegisterComponent<SkeletalMeshComponent>>(*this);
		registry.on_destroy<SkeletalMeshComponent>().disconnect<&MonoScriptSystem::UnregisterComponent<SkeletalMeshComponent>>(*this);
		registry.on_construct<MonoScriptComponent>().disconnect<&MonoScriptSystem::RegisterMonoComponentInRuntime>(*this);
		registry.on_destroy<MonoScriptComponent>().disconnect<&MonoScriptSystem::UnregisterMonoComponent>(*this);

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

	void MonoScriptSystem::BeginDeferredInitComponents() {
		_defferedInitComponents = true;
	}

	void MonoScriptSystem::EndDeferredInitComponents() {
		_defferedInitComponents = false;

		if (_deferredInitComponents.empty()) {
			return;
		}

		for (const auto& entry : _deferredInitComponents) {
			Entity entt = _scene.FindEntityByUUID(entry.monoEntity->GetUUID());
			SetAllComponentFields(*entry.projectComponent, entt.GetComponent<MonoScriptComponent>());
		}

		for (const auto& entry : _deferredInitComponents) {
			entry.projectComponent->CallOnCreate();
		}

		for (const auto& entry : _deferredInitComponents) {
			entry.projectComponent->CallOnStart();
		}

		_deferredInitComponents.clear();
	}
}