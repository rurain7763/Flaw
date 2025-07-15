#include "pch.h"+
#include "TransformSystem.h"
#include "Scene.h"
#include "Components.h"

namespace flaw {
	TransformSystem::TransformSystem(Scene& scene)
		: _scene(scene)
	{
	}

	void TransformSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transform] : registry.view<TransformComponent>().each()) {
			Entity entt(entity, &_scene);

			if (entt.HasParent()) {
				continue;
			} 

			CalculateWorldTransformRecursive(mat4(1.0f), entt);
		}
	}

	void TransformSystem::CalculateWorldTransformRecursive(const mat4& parentTransform, Entity entity, bool calculateAnyway) {
		TransformComponent& transform = entity.GetComponent<TransformComponent>();

		bool shouldCalculate = calculateAnyway || transform.dirty;

		if (shouldCalculate) {
			transform.worldTransform = parentTransform * ModelMatrix(transform.position, transform.rotation, transform.scale);
			transform.dirty = false;
		}

		entity.EachChildren([this, &worldTransform = transform.worldTransform, shouldCalculate](const Entity& child) {
			CalculateWorldTransformRecursive(worldTransform, child, shouldCalculate);
		});
	}

	void TransformSystem::UpdateTransformImmediate(entt::entity entity) {
		Entity entt(entity, &_scene);

		while (entt.HasParent()) {
			entt = entt.GetParent();
		}

		CalculateWorldTransformRecursive(mat4(1.0f), entt, true);
	}
}