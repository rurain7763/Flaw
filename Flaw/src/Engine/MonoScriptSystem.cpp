#include "pch.h"
#include "MonoScriptSystem.h"
#include "Application.h"
#include "Scene.h"
#include "Components.h"
#include "Time/Time.h"

namespace flaw {
	MonoScriptSystem::MonoScriptSystem(Application& app, Scene& scene)
		: _app(app)
		, _scene(scene)
		, _timeSinceStart(0.f)
	{
		Scripting::RegisterMonoScriptSystem(this);
	}

	MonoScriptSystem::~MonoScriptSystem() {
		Scripting::UnregisterMonoScriptSystem(this);
	}

	void MonoScriptSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		auto& monoScriptComp = registry.get<MonoScriptComponent>(entity);

		if (!CreateMonoScriptInstance(enttComp.uuid, monoScriptComp.name.c_str())) {
			return;
		}

		auto& instance = GetMonoScriptInstance(enttComp.uuid);

		if (instance.createMethod) {
			instance.scriptObject.CallMethod(instance.createMethod);
		}

		if (instance.startMethod) {
			instance.scriptObject.CallMethod(instance.startMethod);
		}
	}

	void MonoScriptSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);

		auto it = _monoInstances.find(enttComp.uuid);
		if (it == _monoInstances.end()) {
			return; // Instance does not exist
		}

		auto& instance = it->second;

		if (instance.destroyMethod) {
			instance.scriptObject.CallMethod(instance.destroyMethod);
		}

		_monoInstances.erase(it);
	}

	void MonoScriptSystem::OnStart() {
		auto& registry = _scene.GetRegistry();

		Scripting::SetActiveMonoScriptSystem(this);

		registry.on_construct<MonoScriptComponent>().connect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().connect<&MonoScriptSystem::UnregisterEntity>(*this);

		_timeSinceStart = 0.f;

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoInstances.find(enttComp.uuid);
			if (it == _monoInstances.end()) {
				continue;
			}

			if (it->second.createMethod) {
				it->second.scriptObject.CallMethod(it->second.createMethod);
			}
		}

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoInstances.find(enttComp.uuid);
			if (it == _monoInstances.end()) {
				continue;
			}

			if (it->second.startMethod) {
				it->second.scriptObject.CallMethod(it->second.startMethod);
			}
		}
	}

	void MonoScriptSystem::OnUpdate() {
		for (auto&& [entity, enttComp, monoScriptComp] : _scene.GetRegistry().view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoInstances.find(enttComp.uuid);
			if (it == _monoInstances.end()) {
				continue;
			}

			if (it->second.updateMethod) {
				it->second.scriptObject.CallMethod(it->second.updateMethod);
			}
		}

		_timeSinceStart += Time::DeltaTime();
	}

	void MonoScriptSystem::OnEnd() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, enttComp, monoScriptComp] : registry.view<EntityComponent, MonoScriptComponent>().each()) {
			auto it = _monoInstances.find(enttComp.uuid);
			if (it == _monoInstances.end()) {
				continue;
			}

			if (it->second.destroyMethod) {
				it->second.scriptObject.CallMethod(it->second.destroyMethod);
			}
		}

		registry.on_construct<MonoScriptComponent>().disconnect<&MonoScriptSystem::RegisterEntity>(*this);
		registry.on_destroy<EntityComponent>().disconnect<&MonoScriptSystem::UnregisterEntity>(*this);

		Scripting::SetActiveMonoScriptSystem(nullptr);
	}

	static void CreatePublicFieldsInObjectRecursive(MonoScriptObject& object) {
		object.GetClass().EachFields([&object](std::string_view fieldName, MonoScriptClassField& field) {
			if (field.IsClass()) {
				MonoScriptObject fieldMonoObj(object.GetMonoDomain(), field.GetMonoClass(), field.GetValue<MonoObject*>(&object));
				fieldMonoObj.Instantiate();
				field.SetValue(&object, fieldMonoObj.GetMonoObject());

				CreatePublicFieldsInObjectRecursive(fieldMonoObj);
			}
		});
	}

	bool MonoScriptSystem::CreateMonoScriptInstance(const UUID& uuid, const char* name) {
		auto& domain = Scripting::GetMonoScriptDomain();

		if (!domain.IsClassExists(name)) {
			return false;
		}

		UUID copy = uuid;
		void* args[] = { &copy };

		auto obj = domain.CreateInstance(name, args, 1);
		CreatePublicFieldsInObjectRecursive(obj);

		MonoScriptInstance& instance = _monoInstances[uuid];
		instance.scriptObject = obj;
		instance.createMethod = obj.GetClass().GetMethodRecurcive("OnCreate", 0);
		instance.startMethod = obj.GetClass().GetMethodRecurcive("OnStart", 0);
		instance.updateMethod = obj.GetClass().GetMethodRecurcive("OnUpdate", 0);
		instance.destroyMethod = obj.GetClass().GetMethodRecurcive("OnDestroy", 0);

		return true;
	}

	void MonoScriptSystem::DestroyMonoScriptInstance(const UUID& uuid) {
		_monoInstances.erase(uuid);
	}

	bool MonoScriptSystem::IsMonoScriptInstanceExists(const UUID& uuid) const {
		return _monoInstances.find(uuid) != _monoInstances.end();
	}

	MonoScriptInstance& MonoScriptSystem::GetMonoScriptInstance(const UUID& uuid) {
		auto it = _monoInstances.find(uuid);
		if (it != _monoInstances.end()) {
			return it->second;
		}

		throw std::runtime_error("Mono script instance not found for UUID");
	}

	void MonoScriptSystem::CloneMonoScriptInstances(const std::unordered_map<UUID, MonoScriptInstance>& instances) {
		for (const auto& [uuid, instance] : instances) {
			MonoScriptInstance& newInstance = _monoInstances[uuid];
			newInstance.scriptObject = instance.scriptObject.Clone();
			newInstance.createMethod = instance.createMethod;
			newInstance.startMethod = instance.startMethod;
			newInstance.updateMethod = instance.updateMethod;
			newInstance.destroyMethod = instance.destroyMethod;
		}
	}
}