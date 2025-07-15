#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Entity.h"

namespace flaw {
	class Scene;

	class TransformSystem {
	public:
		TransformSystem(Scene& scene);

		void Update();

		void UpdateTransformImmediate(entt::entity entity);

	private:
		void CalculateWorldTransformRecursive(const mat4& parentTransform, Entity entity, bool calculateAnyway = false);

	private:
		Scene& _scene;
	};
}