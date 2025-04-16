#include "pch.h"
#include "ParticleSystem.h"
#include "Platform.h"
#include "Time/Time.h"
#include "Scene.h"

namespace flaw {
	ParticleSystem::ParticleSystem(Scene& scene) 
		: _scene(scene)
	{
		_computePipeline = Graphics::CreateComputePipeline();
		_computePipeline->SetShader(Graphics::CreateComputeShader("Resources/Shaders/particle.fx"));

		vec3 vertices[] = { vec3(0.0f) };

		VertexBuffer::Descriptor vertexBufferDesc = {};
		vertexBufferDesc.usage = UsageFlag::Static;
		vertexBufferDesc.elmSize = sizeof(vec3);
		vertexBufferDesc.bufferSize = sizeof(vec3);
		vertexBufferDesc.initialData = vertices;

		_vertexBuffer = Graphics::CreateVertexBuffer(vertexBufferDesc);

		uint32_t indices[] = { 0 };

		IndexBuffer::Descriptor indexBufferDesc = {};
		indexBufferDesc.usage = UsageFlag::Static;
		indexBufferDesc.bufferSize = sizeof(uint32_t);
		indexBufferDesc.initialData = indices;

		_indexBuffer = Graphics::CreateIndexBuffer(indexBufferDesc);

		Ref<GraphicsShader> shader = Graphics::CreateGraphicsShader("Resources/Shaders/particle.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel | ShaderCompileFlag::Geometry);
		shader->AddInputElement<float>("POSITION", 3);
		shader->CreateInputLayout();

		_graphicsPipeline = Graphics::CreateGraphicsPipeline();
		_graphicsPipeline->SetShader(shader);
		_graphicsPipeline->SetDepthTest(DepthTest::Less, false);

		_vpMatricesCB = Graphics::CreateConstantBuffer(sizeof(VPMatrices));
		_globalConstantsCB = Graphics::CreateConstantBuffer(sizeof(GlobalConstants));
	}

	void ParticleSystem::Update() {
		auto& registry = _scene.GetRegistry();
		auto& cmdQueue = Graphics::GetCommandQueue();

		GlobalConstants globalContants;

		int32_t width, height;
		Platform::GetFrameBufferSize(width, height);
		globalContants.screenResolution = vec2(static_cast<float>(width), static_cast<float>(height));
		globalContants.time = Time::GetTime();
		globalContants.deltaTime = Time::DeltaTime();

		_globalConstantsCB->Update(&globalContants, sizeof(GlobalConstants));

		for (auto&& [entity, transComp, particleComp] : registry.view<entt::entity, TransformComponent, ParticleComponent>().each()) {
			ParticleResources& compResources = _entityResourceMap[(uint32_t)entity];

			particleComp.timer += Time::DeltaTime();

			_particleUniform.pivotMatrix = transComp.worldTransform;
			_particleUniform.maxParticles = particleComp.maxParticles;
			_particleUniform.spaceType = (uint32_t)particleComp.spaceType;
			_particleUniform.startSpeed = particleComp.startSpeed;
			_particleUniform.startLifeTime = particleComp.startLifeTime;
			_particleUniform.startColor = particleComp.startColor;
			_particleUniform.startSize = particleComp.startSize;

			HandleEmissionModule((uint32_t)entity, compResources, particleComp);
			HandleRandomSpeedModule((uint32_t)entity, compResources, particleComp);
			HandleRandomColorModule((uint32_t)entity, compResources, particleComp);
			HandleColorOverLifetimeModule((uint32_t)entity, compResources, particleComp);
			HandleRandomSizeModule((uint32_t)entity, compResources, particleComp);
			HandleSizeOverLifetimeModule((uint32_t)entity, compResources, particleComp);
			HandleShapeModule((uint32_t)entity, compResources, particleComp);
			HandleNoiseModule((uint32_t)entity, compResources, particleComp);
			HandleRendererModule((uint32_t)entity, compResources, particleComp);

			compResources.particleUniformBuffer->Update(&_particleUniform, sizeof(ParticleUniform));

			cmdQueue.Begin();
			cmdQueue.SetComputePipeline(_computePipeline);
			cmdQueue.SetComputeConstantBuffer(_globalConstantsCB, 1);
			cmdQueue.SetComputeStructuredBuffer(compResources.particleUniformBuffer, BindFlag::UnorderedAccess, 0);
			cmdQueue.SetComputeStructuredBuffer(compResources.particleBuffer, BindFlag::UnorderedAccess, 1);
			
			const uint32_t threadCount = 32;
			cmdQueue.Dispatch((particleComp.maxParticles + threadCount - 1) / threadCount, 1, 1);
			
			cmdQueue.End();

			cmdQueue.Execute();
		}
	}

	void ParticleSystem::HandleEmissionModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::Emission) {
			auto emModule = GetModule<EmissionModule>(entity);
			if (!emModule) {
				emModule = AddModule<EmissionModule>(entity);
			}

			emModule->spawnTimer += Time::DeltaTime();

			const float spanwRate = 1.0f / emModule->spawnOverTime;
			if (emModule->spawnTimer > spanwRate) {
				_particleUniform.spawnCountForThisFrame = static_cast<int32_t>(emModule->spawnTimer / spanwRate);
				emModule->spawnTimer -= spanwRate;
			}
			else {
				_particleUniform.spawnCountForThisFrame = 0;
			}

			// can burst?
			if (emModule->burst && (emModule->burstCycleCount == 0 || emModule->burstCycleCounter < emModule->burstCycleCount)) {
				if (emModule->burstCycleCounter == 0) {
					// when burst not started
					if (emModule->burstStartTime < particleComp.timer) {
						_particleUniform.spawnCountForThisFrame += emModule->burstParticleCount;
						emModule->burstCycleCounter++;
					}
				}
				else if (emModule->burstCycleCounter > 0) {
					// when burst started and cycle
					emModule->burstTimer += Time::DeltaTime();
					if (emModule->burstTimer > emModule->burstCycleInterval) {
						_particleUniform.spawnCountForThisFrame += emModule->burstParticleCount;
						emModule->burstCycleCounter++;
						emModule->burstTimer -= emModule->burstCycleInterval;
					}
				}
			}
		}
		else {
			_particleUniform.spawnCountForThisFrame = 0;
			RemoveModule<EmissionModule>(entity);
		}
	}

	void ParticleSystem::HandleRandomSpeedModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::RandomSpeed) {
			auto rndSpdModule = GetModule<RandomSpeedModule>(entity);
			if (!rndSpdModule) {
				rndSpdModule = AddModule<RandomSpeedModule>(entity);
			}

			_particleUniform.randomSpeedFlag = 1;
			_particleUniform.randomSpeedMin = rndSpdModule->minSpeed;
			_particleUniform.randomSpeedMax = rndSpdModule->maxSpeed;
		}
		else {
			_particleUniform.randomSpeedFlag = 0;
			RemoveModule<RandomSpeedModule>(entity);
		}
	}

	void ParticleSystem::HandleRandomColorModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::RandomColor) {
			auto rcModule = GetModule<RandomColorModule>(entity);
			if (!rcModule) {
				rcModule = AddModule<RandomColorModule>(entity);
			}

			_particleUniform.randomColorFlag = 1;
			_particleUniform.randomColorMin = rcModule->minColor;
			_particleUniform.randomColorMax = rcModule->maxColor;
		}
		else {
			_particleUniform.randomColorFlag = 0;
			RemoveModule<RandomColorModule>(entity);
		}
	}

	void ParticleSystem::HandleColorOverLifetimeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::ColorOverLifetime) {
			auto coltModule = GetModule<ColorOverLifetimeModule>(entity);
			if (!coltModule) {
				coltModule = AddModule<ColorOverLifetimeModule>(entity);
			}

			_particleUniform.coltFlag = 1;
			_particleUniform.coltEasing = (uint32_t)coltModule->easing;
			_particleUniform.coltEasingStartRatio = coltModule->easingStartRatio;
			_particleUniform.coltRedFactorRange = coltModule->redFactorRange;
			_particleUniform.coltGreenFactorRange = coltModule->greenFactorRange;
			_particleUniform.coltBlueFactorRange = coltModule->blueFactorRange;
			_particleUniform.coltAlphaFactorRange = coltModule->alphaFactorRange;
		}
		else {
			_particleUniform.coltFlag = 0;
			RemoveModule<ColorOverLifetimeModule>(entity);
		}
	}

	void ParticleSystem::HandleRandomSizeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::RandomSize) {
			auto rsModule = GetModule<RandomSizeModule>(entity);
			if (!rsModule) {
				rsModule = AddModule<RandomSizeModule>(entity);
			}

			_particleUniform.randomSizeFlag = 1;
			_particleUniform.randomSizeMin = rsModule->minSize;
			_particleUniform.randomSizeMax = rsModule->maxSize;
		}
		else {
			_particleUniform.randomSizeFlag = 0;
			RemoveModule<RandomSizeModule>(entity);
		}
	}

	void ParticleSystem::HandleSizeOverLifetimeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::SizeOverLifetime) {
			auto soltModule = GetModule<SizeOverLifetimeModule>(entity);
			if (!soltModule) {
				soltModule = AddModule<SizeOverLifetimeModule>(entity);
			}

			_particleUniform.soltFlag = 1;
			_particleUniform.soltEasing = (uint32_t)soltModule->easing;
			_particleUniform.soltEasingStartRatio = soltModule->easingStartRatio;
			_particleUniform.soltFactorRange = soltModule->sizeFactorRange;
		}
		else {
			_particleUniform.soltFlag = 0;
			RemoveModule<SizeOverLifetimeModule>(entity);
		}
	}

	void ParticleSystem::HandleShapeModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::Shape) {
			auto shapeModule = GetModule<ShapeModule>(entity);
			if (!shapeModule) {
				shapeModule = AddModule<ShapeModule>(entity);
			}

			_particleUniform.shapeType = uint32_t(shapeModule->shapeType);

			if (shapeModule->shapeType == ShapeModule::ShapeType::Sphere) {
				_particleUniform.shapeFData0 = shapeModule->sphere.radius;
				_particleUniform.shapeFData1 = shapeModule->sphere.thickness;
			}
			else if (shapeModule->shapeType == ShapeModule::ShapeType::Box) {
				_particleUniform.shapeVData0 = shapeModule->box.size;
				_particleUniform.shapeVData1 = shapeModule->box.thickness;
			}
		}
		else {
			_particleUniform.shapeType = 0;
			RemoveModule<ShapeModule>(entity);
		}
	}

	void ParticleSystem::HandleNoiseModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::Noise) {
			auto noiseModule = GetModule<NoiseModule>(entity);
			if (!noiseModule) {
				noiseModule = AddModule<NoiseModule>(entity);
			}

			_particleUniform.noiseFlag = 1;
			_particleUniform.noiseStrength = noiseModule->strength;
			_particleUniform.noiseFrequency = noiseModule->frequency;
		}
		else {
			_particleUniform.noiseFlag = 0;
			RemoveModule<NoiseModule>(entity);
		}
	}

	void ParticleSystem::HandleRendererModule(uint32_t entity, ParticleResources& compResources, ParticleComponent& particleComp) {
		if (particleComp.modules & ParticleComponent::ModuleType::Renderer) {
			auto rendererModule = GetModule<RendererModule>(entity);
			if (!rendererModule) {
				rendererModule = AddModule<RendererModule>(entity);
			}

			_particleUniform.rendererFlag = 1;
			_particleUniform.rendererAlignment = uint32_t(rendererModule->alignment);
		}
		else {
			_particleUniform.rendererFlag = 0;
			RemoveModule<RendererModule>(entity);
		}
	}

	void ParticleSystem::Render(const mat4& viewMatrix, const mat4& projectionMatrix) {
		auto& registry = _scene.GetRegistry();
		auto& cmdQueue = Graphics::GetCommandQueue();

		VPMatrices vpMatrices;
		vpMatrices.view = viewMatrix;
		vpMatrices.projection = projectionMatrix;
		_vpMatricesCB->Update(&vpMatrices, sizeof(VPMatrices));

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::PointList);
		cmdQueue.SetPipeline(_graphicsPipeline);
		cmdQueue.SetConstantBuffer(_vpMatricesCB, 0);
		cmdQueue.SetConstantBuffer(_globalConstantsCB, 1);
		cmdQueue.SetVertexBuffer(_vertexBuffer);

		for (auto&& [entity, particleComp] : registry.view<entt::entity, ParticleComponent>().each()) {
			auto it = _entityResourceMap.find((uint32_t)entity);
			if (it == _entityResourceMap.end()) {
				continue;
			}

			auto& compResources = it->second;
			cmdQueue.SetStructuredBuffer(compResources.particleUniformBuffer, 0);
			cmdQueue.SetStructuredBuffer(compResources.particleBuffer, 1);

			// TODO: set particle texture

			cmdQueue.DrawIndexedInstanced(_indexBuffer, 1, particleComp.maxParticles);
		}

		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();
	}

	void ParticleSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		auto it = _entityResourceMap.find((uint32_t)entity);

		if (it == _entityResourceMap.end()) {
			StructuredBuffer::Descriptor particleUniformSBDesc = {};
			particleUniformSBDesc.elmSize = sizeof(ParticleUniform);
			particleUniformSBDesc.count = 1;
			particleUniformSBDesc.bindFlags = BindFlag::UnorderedAccess | BindFlag::ShaderResource;
			particleUniformSBDesc.accessFlags = AccessFlag::Write;

			std::vector<Particle> particles(MaxParticleBufferSize);

			StructuredBuffer::Descriptor particleSBDesc = {};
			particleSBDesc.elmSize = sizeof(Particle);
			particleSBDesc.count = particles.size();
			particleSBDesc.bindFlags = BindFlag::UnorderedAccess | BindFlag::ShaderResource;
			particleSBDesc.initialData = particles.data();

			_entityResourceMap[(uint32_t)entity] = {
				std::unordered_map<ParticleComponent::ModuleType, Ref<Module>>(),
				Graphics::CreateStructuredBuffer(particleUniformSBDesc),
				Graphics::CreateStructuredBuffer(particleSBDesc)
			};
		}
	}

	void ParticleSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_entityResourceMap.erase((uint32_t)entity);
	}
}