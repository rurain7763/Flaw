#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "RenderQueue.h"
#include "Camera.h"

namespace flaw {
	constexpr static uint32_t CascadeShadowCount = 3;

	class Scene;
	struct CameraRenderStage;

	struct ShadowUniforms {
		uint32_t lightVPMatrixCount = 0;

		uint32_t padding[3]; // Padding to align to 16 bytes
	};

	struct LightVPMatrix {
		mat4 view;
		mat4 projection;
	};

	struct DirectionalLightShadowMap {
		vec3 lightDirection;
		std::array<LightVPMatrix, CascadeShadowCount> lightVPMatrices;
		std::array<float, CascadeShadowCount> cascadeDistances; // Split distances for each cascade
		std::array<Ref<GraphicsRenderPass>, CascadeShadowCount> renderPasses;
	};

	struct SpotLightShadowMap {
		LightVPMatrix lightVPMatrix;
		Ref<GraphicsRenderPass> renderPass;
	};

	struct PointLightShadowMap {
		std::array<LightVPMatrix, 6> lightVPMatrices;
		Ref<GraphicsRenderPass> renderPass;
	};

	class ShadowSystem {
	public:
		ShadowSystem(Scene& scene);
		~ShadowSystem();

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		void Update();
		void Render(CameraRenderStage& stage, Ref<StructuredBuffer>& batchedTransformSB);

		DirectionalLightShadowMap& GetDirectionalLightShadowMap(const entt::entity id) { return _directionalShadowMaps[id]; }
		SpotLightShadowMap& GetSpotLightShadowMap(const entt::entity id) { return _spotLightShadowMaps[id]; }
		PointLightShadowMap& GetPointLightShadowMap(const entt::entity id) { return _pointLightShadowMaps[id]; }

	private:
		Ref<GraphicsRenderPass> CreateDirectionalLightShadowMapRenderPass(uint32_t width, uint32_t height);
		Ref<GraphicsRenderPass> CreateSpotLightShadowMapRenderPass();
		Ref<GraphicsRenderPass> CreatePointLightShadowMapRenderPass();

		std::vector<Frustum::Corners> GetCascadeFrustumCorners(const Frustum& frustum);
		void CalcTightDirectionalLightMatrices(const Frustum::Corners& worldSpaceCorners, const vec3& lightDirection, mat4& outView, mat4& outProjection);

		void DrawRenderEntry(const RenderEntry& entry, Ref<StructuredBuffer>& batchedTransformSB, const LightVPMatrix* lightVPMatrices, int32_t lightVPMatrixCount);

	private:
		constexpr static uint32_t ShadowMapSize = 2048;
		constexpr static uint32_t MaxLightVPCount = 6;

		Scene& _scene;

		Ref<Material> _shadowMapStaticMaterial;
		Ref<Material> _shadowMapSkeletalMaterial;

		Ref<ConstantBuffer> _shadowUniformsCB;
		Ref<StructuredBuffer> _lightVPMatricesSB;

		RenderQueue _shadowMapRenderQueue;

		std::unordered_map<entt::entity, DirectionalLightShadowMap> _directionalShadowMaps;
		std::unordered_map<entt::entity, SpotLightShadowMap> _spotLightShadowMaps;
		std::unordered_map<entt::entity, PointLightShadowMap> _pointLightShadowMaps;
	};
}