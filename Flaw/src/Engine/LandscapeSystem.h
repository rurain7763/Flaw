#pragma once

#include "Core.h"	
#include "Graphics.h"
#include "RenderQueue.h"
#include "ECS/ECS.h"
#include "Utils/UUID.h"

namespace flaw {
	class Scene;
	
	struct Landscape {
		Ref<Mesh> mesh;
		Ref<Material> material;
	};

	class LandscapeSystem {
	public:
		LandscapeSystem(Scene& scene);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Update();
		void Render(const Camera& camera, RenderQueue& renderQueue);

		Landscape& GetLandscape(UUID landscapeID) { return _landscapes.at(landscapeID); }

	private:
		Ref<Mesh> CreateLandscapeMesh(uint32_t tilingX, uint32_t tilingY);

	private:
		constexpr static int32_t TilingXIndex = 0;
		constexpr static int32_t TilingYIndex = 1;

		Scene& _scene;

		Ref<GraphicsShader> _landscapeShader;

		std::map<UUID, Landscape> _landscapes;
	};
}