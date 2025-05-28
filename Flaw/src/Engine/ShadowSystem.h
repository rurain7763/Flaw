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

	struct DirectionalLightShadowMap {
		ShadowUniforms uniforms;
		Ref<GraphicsRenderPass> renderPass;
	};

	struct SpotLightShadowMap {
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

		DirectionalLightShadowMap& GetDirectionalLightShadowMap(const uint32_t& id) { return _directionalShadowMaps[id]; }
		SpotLightShadowMap& GetSpotLightShadowMap(const uint32_t& id) { return _spotLightShadowMaps[id]; }

	private:
		Ref<GraphicsRenderPass> CreateShadowMapRenderPass();
		
		void DrawRenderEntry(const RenderEntry& entry, const ShadowUniforms& shadowUniforms, Ref<StructuredBuffer>& batchedTransformSB);

	private:
		constexpr static uint32_t ShadowMapSize = 2048;

		Scene& _scene;

		Ref<Material> _shadowMapStaticMaterial;
		Ref<Material> _shadowMapSkeletalMaterial;

		Ref<ConstantBuffer> _shadowUniformsCB;

		RenderQueue _shadowMapRenderQueue;

		std::unordered_map<uint32_t, DirectionalLightShadowMap> _directionalShadowMaps;
		std::unordered_map<uint32_t, SpotLightShadowMap> _spotLightShadowMaps;
	};
}