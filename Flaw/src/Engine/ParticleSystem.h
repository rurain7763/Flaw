#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"
#include "Components.h"
#include "Entity.h"
#include "Utils/Easing.h"

namespace flaw {
	class Scene;

	struct ParticleUniform {
		int32_t spawnCountForThisFrame = 0;
		mat4 pivotMatrix = mat4(1.0f);

		int32_t maxParticles = 0;
		int32_t spaceType = 0;

		float startSpeed = 0.0f;

		int32_t randomSpeedFlag = 0;
		float randomSpeedMin = 0.0f;
		float randomSpeedMax = 0.0f;

		float startLifeTime = 0.0f;

		vec4 startColor = vec4(1.0f);

		int32_t randomColorFlag = 0;
		vec4 randomColorMin = vec4(1.0f);
		vec4 randomColorMax = vec4(1.0f);

		vec3 startSize = vec3(1.0f);

		int32_t randomSizeFlag = 0;
		vec3 randomSizeMin = vec3(1.0f);
		vec3 randomSizeMax = vec3(1.0f);

		int32_t shapeType = 0;
		float shapeFData0 = 0.0f;
		float shapeFData1 = 0.0f;
		vec3 shapeVData0 = vec3(0.0f);
		vec3 shapeVData1 = vec3(0.0f);

		// solt = size over lifetime
		int32_t soltFlag = 0;
		int32_t soltEasing = 0;
		float soltEasingStartRatio = 0.0f;
		vec2 soltFactorRange = vec2(0.0f, 1.0f);

		// colt = color over lifetime
		int32_t coltFlag = 0;
		int32_t coltEasing = 0;
		float coltEasingStartRatio = 0.0f;
		vec2 coltRedFactorRange = vec2(0.0f, 1.0f);
		vec2 coltGreenFactorRange = vec2(0.0f, 1.0f);
		vec2 coltBlueFactorRange = vec2(0.0f, 1.0f);
		vec2 coltAlphaFactorRange = vec2(0.0f, 1.0f);

		int32_t noiseFlag = 0;
		float noiseStrength = 1.0f;
		float noiseFrequency = 1.0f;

		int32_t rendererFlag = 0;
		int32_t rendererAlignment = 0;
	};

	struct Particle {
		int32_t active = 0;
		vec3 localPosition = vec3(0.0f);
		vec3 worldPosition = vec3(0.0f);
		vec3 size = vec3(1.0f);
		vec3 velocity = vec3(0.0f);
		vec4 color = vec4(1.0);
		float lifeTime = 0.0f;

		float lifeTimeStart = 0.0f;
		float noiseStart = 0.0f;
		vec3 sizeStart = vec3(1.0f);
		vec4 colorStart = vec4(1.0f);
	};

	struct Module {
		virtual ~Module() = default;
	};

	struct EmissionModule : Module {
		int32_t spawnOverTime = 10;

		bool burst = false;
		float burstStartTime = 0.0f;
		uint32_t burstParticleCount = 0;
		uint32_t burstCycleCount = 0; // 0 means infinite
		float burstCycleInterval = 0.0f;

		float spawnTimer = 0.0f;
		float burstTimer = 0.0f;
		uint32_t burstCycleCounter = 0;
	};

	struct ShapeModule : Module {
		enum class ShapeType {
			None,
			Sphere,
			Box
		};

		ShapeType shapeType = ShapeType::Sphere;

		union {
			struct {
				float radius;
				float thickness;
			} sphere;

			struct {
				vec3 size;
				vec3 thickness;
			} box;

			// TODO: add other shapes
		};
	};

	struct RandomSpeedModule : Module {
		float minSpeed = 0.5f;
		float maxSpeed = 1.0f;
	};

	struct RandomColorModule : Module {
		vec4 minColor = vec4(1.0f);
		vec4 maxColor = vec4(1.0f);
	};

	struct RandomSizeModule : Module {
		vec3 minSize = vec3(1.0f);
		vec3 maxSize = vec3(1.0f);
	};

	struct ColorOverLifetimeModule : Module {
		Easing easing = Easing::Linear;
		float easingStartRatio = 0.0f;
		vec2 redFactorRange = vec2(1.0f, 1.0f);
		vec2 greenFactorRange = vec2(1.0f, 1.0f);
		vec2 blueFactorRange = vec2(1.0f, 1.0f);
		vec2 alphaFactorRange = vec2(1.0f, 1.0f);
	};

	struct SizeOverLifetimeModule : Module {
		Easing easing = Easing::Linear;
		float easingStartRatio = 0.0f;
		vec2 sizeFactorRange = vec2(0.0f, 1.0f);
	};

	struct NoiseModule : Module {
		float strength = 1.0f;
		float frequency = 1.0f;
	};

	struct RendererModule : Module {
		enum class Alignment {
			View,
			Velocity
		};

		Alignment alignment = Alignment::View;
	};

	struct ParticleResources {
		std::unordered_map<ParticleComponent::ModuleType, Ref<Module>> modules;
		Ref<StructuredBuffer> particleUniformBuffer;
		Ref<StructuredBuffer> particleBuffer;
	};

	class ParticleSystem {
	public:
		ParticleSystem(Scene& scene);

		void Update();
		void Render(const mat4& viewMatrix, const mat4& projectionMatrix);

		void RegisterEntity(entt::registry& registry, entt::entity entity);
		void UnregisterEntity(entt::registry& registry, entt::entity entity);

		template<typename T>
		ParticleComponent::ModuleType GetModuleType() {
			if constexpr (std::is_same_v<T, EmissionModule>) {
				return ParticleComponent::ModuleType::Emission;
			}
			else if constexpr (std::is_same_v<T, ShapeModule>) {
				return ParticleComponent::ModuleType::Shape;
			}
			else if constexpr (std::is_same_v<T, RandomSpeedModule>) {
				return ParticleComponent::ModuleType::RandomSpeed;
			}
			else if constexpr (std::is_same_v<T, RandomColorModule>) {
				return ParticleComponent::ModuleType::RandomColor;
			}
			else if constexpr (std::is_same_v<T, RandomSizeModule>) {
				return ParticleComponent::ModuleType::RandomSize;
			}
			else if constexpr (std::is_same_v<T, ColorOverLifetimeModule>) {
				return ParticleComponent::ModuleType::ColorOverLifetime;
			}
			else if constexpr (std::is_same_v<T, SizeOverLifetimeModule>) {
				return ParticleComponent::ModuleType::SizeOverLifetime;
			}
			else if constexpr (std::is_same_v<T, NoiseModule>) {
				return ParticleComponent::ModuleType::Noise;
			}
			else if constexpr (std::is_same_v<T, RendererModule>) {
				return ParticleComponent::ModuleType::Renderer;
			}
		}

		template<typename T>
		Ref<T> AddModule(const uint32_t entity) {
			auto it = _entityResourceMap.find(entity);
			if (it != _entityResourceMap.end()) {
				auto& compResources = it->second;
				auto moduleType = GetModuleType<T>();
				if (compResources.modules.find(moduleType) == compResources.modules.end()) {
					auto ret = std::make_shared<T>();
					compResources.modules[moduleType] = ret;
					return ret;
				}
			}
			return nullptr;
		}

		template<typename T>
		void RemoveModule(const uint32_t entity) {
			auto it = _entityResourceMap.find(entity);
			if (it != _entityResourceMap.end()) {
				auto& compResources = it->second;
				auto moduleType = GetModuleType<T>();
				compResources.modules.erase(moduleType);
			}
		}

		template<typename T>
		Ref<T> GetModule(const uint32_t entity) {
			auto it = _entityResourceMap.find(entity);
			if (it != _entityResourceMap.end()) {
				auto& compResources = it->second;
				auto moduleType = GetModuleType<T>();
				auto moduleIt = compResources.modules.find(moduleType);
				if (moduleIt != compResources.modules.end()) {
					return std::static_pointer_cast<T>(moduleIt->second);
				}
			}
			return nullptr;
		}

	private:
		void HandleEmissionModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleRandomSpeedModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleRandomColorModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleColorOverLifetimeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleRandomSizeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleSizeOverLifetimeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleShapeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleNoiseModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);
		void HandleRendererModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp);

	private:
		constexpr static uint32_t MaxParticleBufferSize = 1024;

		Scene& _scene;

		Ref<ComputePipeline> _computePipeline;

		Ref<VertexBuffer> _vertexBuffer;
		Ref<IndexBuffer> _indexBuffer;
		Ref<GraphicsPipeline> _graphicsPipeline;

		Ref<ConstantBuffer> _vpMatricesCB;
		Ref<ConstantBuffer> _globalConstantsCB;

		ParticleUniform _particleUniform;

		std::unordered_map<uint32_t, ParticleResources> _entityResourceMap;
	};
}