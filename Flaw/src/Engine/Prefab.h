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

		static std::vector<int8_t> ExportData(Entity entity);

	private:
		std::unordered_map<UUID, std::function<void(Entity&)>> _componentCreators;
		std::unordered_map<UUID, UUID> _parentMap;
	};
}