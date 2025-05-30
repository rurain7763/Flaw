#pragma once

#include "Core.h"	
#include "Graphics.h"
#include "RenderQueue.h"
#include "ECS/ECS.h"
#include "Utils/UUID.h"
#include "Mesh.h"
#include "Camera.h"

namespace flaw {
	class Scene;
	struct CameraRenderStage;
	
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
		void GatherRenderable(CameraRenderStage& stage);

		Landscape& GetLandscape(uint32_t landscapeID) { return _landscapes.at(landscapeID); }

	private:
		Ref<Mesh> CreateLandscapeMesh(uint32_t tilingX, uint32_t tilingY);

	private:
		constexpr static int32_t TilingXIndex = 0;
		constexpr static int32_t TilingYIndex = 1;

		Scene& _scene;

		Ref<GraphicsShader> _landscapeShader;

		std::map<uint32_t, Landscape> _landscapes;
	};
}