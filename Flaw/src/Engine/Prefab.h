#pragma once

#include "Core.h"
#include "Entity.h"

namespace flaw {
	class Scene;

	class Prefab {
	public:
		Prefab() = default;
		Prefab(const int8_t* data);

		Entity CreateEntity(Scene& scene);
		Entity CreateEntity(Scene& scene, const vec3& position, const vec3& rotation = vec3(0.f), const vec3& scale = vec3(1.f));

		static std::vector<int8_t> ExportData(Entity entity);

	private:
		std::unordered_map<UUID, std::function<void(Entity&)>> _componentCreators;
		std::unordered_map<UUID, UUID> _parentMap;
	};
}