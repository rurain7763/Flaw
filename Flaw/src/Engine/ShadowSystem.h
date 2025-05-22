#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "RenderQueue.h"

namespace flaw {
	class Scene;

	struct ShadowUniforms {
		mat4 view;
		mat4 projection;
	};

	struct ShadowMap {
		ShadowUniforms uniforms;
		Ref<GraphicsRenderPass> renderPass;
	};

	class ShadowSystem {
	public:
		ShadowSystem(Scene& scene);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Update();
		void Render(Ref<StructuredBuffer>& batchedTransformSB);

		ShadowMap& GetShadowMap(const uint32_t& uuid) { return _shadowMaps[uuid]; }

	private:
		constexpr static uint32_t ShadowMapSize = 2048;

		Scene& _scene;

		Ref<Material> _shadowMapMaterial;

		Ref<ConstantBuffer> _shadowUniformsCB;

		RenderQueue _shadowMapRenderQueue;

		std::unordered_map<uint32_t, ShadowMap> _shadowMaps;
	};
}