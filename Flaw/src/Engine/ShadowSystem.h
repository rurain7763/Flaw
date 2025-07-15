#pragma once

#include "Core.h"
#include "ECS/ECS.h"
#include "Graphics.h"
#include "Utils/UUID.h"
#include "RenderQueue.h"
#include "Camera.h"
#include "Components.h"

namespace flaw {
	constexpr static uint32_t CascadeShadowCount = 3;

	class Scene;

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

		void Update();
		void Render(const vec3& cameraPos, const Frustum& cameraFrustum);

		DirectionalLightShadowMap& GetDirectionalLightShadowMap(const entt::entity id) { return _directionalShadowMaps[id]; }
		SpotLightShadowMap& GetSpotLightShadowMap(const entt::entity id) { return _spotLightShadowMaps[id]; }
		PointLightShadowMap& GetPointLightShadowMap(const entt::entity id) { return _pointLightShadowMaps[id]; }

	private:
		template<typename T>
		void RegisterEntity(entt::registry& registry, entt::entity entity) {
			if constexpr (std::is_same_v<T, DirectionalLightComponent>) {
				auto& shadowMap = _directionalShadowMaps[entity];

				for (int32_t i = 0; i < CascadeShadowCount; ++i) {
					shadowMap.renderPasses[i] = CreateDirectionalLightShadowMapRenderPass(ShadowMapSize >> i, ShadowMapSize >> i);
				}
			}
			else if (std::is_same_v<T, SpotLightComponent>) {
				auto& shadowMap = _spotLightShadowMaps[entity];
				shadowMap.renderPass = CreateSpotLightShadowMapRenderPass();
			}
			else if (std::is_same_v<T, PointLightComponent>) {
				auto& shadowMap = _pointLightShadowMaps[entity];
				shadowMap.renderPass = CreatePointLightShadowMapRenderPass();
			}
		}

		template<typename T>
		void UnregisterEntity(entt::registry& registry, entt::entity entity) {
			if constexpr (std::is_same_v<T, DirectionalLightComponent>) {
				_directionalShadowMaps.erase(entity);
			}
			else if constexpr (std::is_same_v<T, SpotLightComponent>) {
				_spotLightShadowMaps.erase(entity);
			}
			else if constexpr (std::is_same_v<T, PointLightComponent>) {
				_pointLightShadowMaps.erase(entity);
			}
		}

		Ref<GraphicsRenderPass> CreateDirectionalLightShadowMapRenderPass(uint32_t width, uint32_t height);
		Ref<GraphicsRenderPass> CreateSpotLightShadowMapRenderPass();
		Ref<GraphicsRenderPass> CreatePointLightShadowMapRenderPass();

		std::vector<Frustum::Corners> GetCascadeFrustumCorners(const Frustum& frustum);
		void CalcTightDirectionalLightMatrices(const Frustum::Corners& worldSpaceCorners, const vec3& lightDirection, mat4& outView, mat4& outProjection);

		void DrawRenderEntry(const RenderEntry& entry, const LightVPMatrix* lightVPMatrices, int32_t lightVPMatrixCount);

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