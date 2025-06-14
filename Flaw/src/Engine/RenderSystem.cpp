#include "pch.h"
#include "RenderSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Time/Time.h"
#include "Graphics/GraphicsFunc.h"
#include "LandscapeSystem.h"
#include "AnimationSystem.h"
#include "ShadowSystem.h"
#include "SkyBoxSystem.h"

// TODO: remove this
#include "Renderer2D.h"
#include "ParticleSystem.h"

namespace flaw {
	RenderSystem::RenderSystem(Scene& scene)
		: _scene(scene)
	{
		CreateRenderPasses();
		CreateConstantBuffers();
		CreateStructuredBuffers();
	}

	void RenderSystem::CreateRenderPasses() {
		int32_t width, height;
		Graphics::GetSize(width, height);

		// object MRT
		GraphicsRenderPass::Descriptor objMRTDesc = {};
		objMRTDesc.renderTargets.resize(GeometryRTCount);

		Texture2D::Descriptor texDesc = {};
		texDesc.width = width;
		texDesc.height = height;
		texDesc.usage = UsageFlag::Static;

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryPosition].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryPosition].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryPosition].viewportX = 0;
		objMRTDesc.renderTargets[GeometryPosition].viewportY = 0;
		objMRTDesc.renderTargets[GeometryPosition].viewportWidth = width;
		objMRTDesc.renderTargets[GeometryPosition].viewportHeight = height;
		objMRTDesc.renderTargets[GeometryPosition].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		objMRTDesc.renderTargets[GeometryPosition].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryNormal].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryNormal].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryNormal].viewportX = 0;
		objMRTDesc.renderTargets[GeometryNormal].viewportY = 0;
		objMRTDesc.renderTargets[GeometryNormal].viewportWidth = width;
		objMRTDesc.renderTargets[GeometryNormal].viewportHeight = height;
		objMRTDesc.renderTargets[GeometryNormal].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		objMRTDesc.renderTargets[GeometryNormal].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryAlbedo].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryAlbedo].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryAlbedo].viewportX = 0;
		objMRTDesc.renderTargets[GeometryAlbedo].viewportY = 0;
		objMRTDesc.renderTargets[GeometryAlbedo].viewportWidth = width;
		objMRTDesc.renderTargets[GeometryAlbedo].viewportHeight = height;
		objMRTDesc.renderTargets[GeometryAlbedo].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		objMRTDesc.renderTargets[GeometryAlbedo].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryMaterial].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryMaterial].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryMaterial].viewportX = 0;
		objMRTDesc.renderTargets[GeometryMaterial].viewportY = 0;
		objMRTDesc.renderTargets[GeometryMaterial].viewportWidth = width;
		objMRTDesc.renderTargets[GeometryMaterial].viewportHeight = height;
		objMRTDesc.renderTargets[GeometryMaterial].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		objMRTDesc.renderTargets[GeometryMaterial].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryEmissive].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryEmissive].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryEmissive].viewportX = 0;
		objMRTDesc.renderTargets[GeometryEmissive].viewportY = 0;
		objMRTDesc.renderTargets[GeometryEmissive].viewportWidth = width;
		objMRTDesc.renderTargets[GeometryEmissive].viewportHeight = height;
		objMRTDesc.renderTargets[GeometryEmissive].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		objMRTDesc.renderTargets[GeometryEmissive].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		objMRTDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		objMRTDesc.depthStencil.resizeFunc = [](GraphicsDepthStencil& current, int32_t width, int32_t height) {
			current.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		};

		_geometryPass = Graphics::CreateRenderPass(objMRTDesc);

		// decal pass
		GraphicsRenderPass::Descriptor decalPassDesc = {};
		decalPassDesc.renderTargets.resize(DecalRTCount);

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		decalPassDesc.renderTargets[DecalAlbedo].blendMode = BlendMode::Alpha;
		decalPassDesc.renderTargets[DecalAlbedo].alphaToCoverage = true;
		decalPassDesc.renderTargets[DecalAlbedo].texture = _geometryPass->GetRenderTargetTex(GeometryAlbedo);
		decalPassDesc.renderTargets[DecalAlbedo].viewportX = 0;
		decalPassDesc.renderTargets[DecalAlbedo].viewportY = 0;
		decalPassDesc.renderTargets[DecalAlbedo].viewportWidth = width;
		decalPassDesc.renderTargets[DecalAlbedo].viewportHeight = height;
		decalPassDesc.renderTargets[DecalAlbedo].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		decalPassDesc.renderTargets[DecalAlbedo].resizeFunc = [this](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			current.texture = _geometryPass->GetRenderTargetTex(GeometryAlbedo);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		decalPassDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		decalPassDesc.depthStencil.resizeFunc = [](GraphicsDepthStencil& current, int32_t width, int32_t height) {
			current.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		};

		_decalPass = Graphics::CreateRenderPass(decalPassDesc);

		// light MRT
		GraphicsRenderPass::Descriptor lightMRTDesc = {};
		lightMRTDesc.renderTargets.resize(LightingRTCount);

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingRadiance].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingRadiance].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingRadiance].viewportX = 0;
		lightMRTDesc.renderTargets[LightingRadiance].viewportY = 0;
		lightMRTDesc.renderTargets[LightingRadiance].viewportWidth = width;
		lightMRTDesc.renderTargets[LightingRadiance].viewportHeight = height;
		lightMRTDesc.renderTargets[LightingRadiance].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		lightMRTDesc.renderTargets[LightingRadiance].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingShadow].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingShadow].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingShadow].viewportX = 0;
		lightMRTDesc.renderTargets[LightingShadow].viewportY = 0;
		lightMRTDesc.renderTargets[LightingShadow].viewportWidth = width;
		lightMRTDesc.renderTargets[LightingShadow].viewportHeight = height;
		lightMRTDesc.renderTargets[LightingShadow].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		lightMRTDesc.renderTargets[LightingShadow].resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
			desc.format = PixelFormat::RGBA32F;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		_lightingPass = Graphics::CreateRenderPass(lightMRTDesc);
	}

	void RenderSystem::CreateConstantBuffers() {
		_vpCB = Graphics::CreateConstantBuffer(sizeof(CameraConstants));
		_globalCB = Graphics::CreateConstantBuffer(sizeof(GlobalConstants));
		_lightCB = Graphics::CreateConstantBuffer(sizeof(LightConstants));
		_materialCB = Graphics::CreateConstantBuffer(sizeof(MaterialConstants));
		_directionalLightUniformCB = Graphics::CreateConstantBuffer(sizeof(DirectionalLightUniforms));
		_spotLightUniformCB = Graphics::CreateConstantBuffer(sizeof(SpotLightUniforms));
		_pointLightUniformCB = Graphics::CreateConstantBuffer(sizeof(PointLightUniforms));
	}

	void RenderSystem::CreateStructuredBuffers() {
		// Create a structured buffer for batched transforms
		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(mat4);
		sbDesc.count = MaxBatchTransformCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		_batchedTransformSB = Graphics::CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for decals
		sbDesc.elmSize = sizeof(Decal);
		sbDesc.count = MaxDecalCount;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		_decalSB = Graphics::CreateStructuredBuffer(sbDesc);
	}

	void RenderSystem::Update() {
		// set camera render stages
		_renderStages.clear();
		for (auto&& [entity, transformComp, cameraComp] : _scene.GetRegistry().view<TransformComponent, CameraComponent>().each()) {
			const vec3 position = transformComp.GetWorldPosition();
			const vec3 lookDirection = transformComp.GetWorldFront();

			CameraRenderStage stage;
			stage.cameraPosition = position;
			stage.viewMatrix = LookAt(position, position + lookDirection, Up);

			if (cameraComp.perspective) {
				stage.projectionMatrix = Perspective(cameraComp.fov, cameraComp.aspectRatio, cameraComp.nearClip, cameraComp.farClip);
				CreateFrustum(GetFovX(cameraComp.fov, cameraComp.aspectRatio), cameraComp.fov, cameraComp.nearClip, cameraComp.farClip, position, lookDirection, stage.frustum);
			}
			else {
				const float height = cameraComp.orthoSize;
				const float width = height * cameraComp.aspectRatio;
				stage.projectionMatrix = Orthographic(-width, width, -height, height, cameraComp.nearClip, cameraComp.farClip);
				// TODO: orthographic frustum
			}

			_renderStages.emplace(cameraComp.depth, std::move(stage));
		}

		UpdateSystems();
	}

	void RenderSystem::Update(Ref<Camera> camera) {
		// set camera render stages
		_renderStages.clear();
		CameraRenderStage stage;
		stage.cameraPosition = camera->GetPosition();
		stage.viewMatrix = camera->GetViewMatrix();
		stage.projectionMatrix = camera->GetProjectionMatrix();
		stage.frustum = camera->GetFrustum();
		_renderStages.emplace(0, std::move(stage));

		UpdateSystems();
	}

	void RenderSystem::UpdateSystems() {
		int32_t width, height;
		Graphics::GetSize(width, height);

		_globalConstants.screenResolution = vec2((float)width, (float)height);
		_globalConstants.time = Time::GetTime();
		_globalConstants.deltaTime = Time::DeltaTime();
		_globalCB->Update(&_globalConstants, sizeof(GlobalConstants));

		_scene.GetAnimationSystem().Update();
		_scene.GetLandscapeSystem().Update();
		_scene.GetParticleSystem().Update(_globalCB);
		_scene.GetSkyBoxSystem().Update();
		_scene.GetShadowSystem().Update();

		GatherLights();
		GatherDecals();
		GatherRenderableObjects();
	}

	void RenderSystem::GatherLights() {
		auto& enttRegistry = _scene.GetRegistry();
		auto& shadowSystem = _scene.GetShadowSystem();

		_lightConstants = {};

		for (auto&& [entity, transform, skyLightComp] : enttRegistry.view<TransformComponent, SkyLightComponent>().each()) {
			// Skylight's 하나만 존재해야 함
			_lightConstants.ambientColor = skyLightComp.color;
			_lightConstants.ambientIntensity = skyLightComp.intensity;
			break;
		}

		_lightCB->Update(&_lightConstants, sizeof(LightConstants));
	}

	void RenderSystem::GatherDecals() {
		_decals.clear();
		_decalTextureIndexMap.clear();
		_decalTextures.clear();
		for (auto&& [entity, transform, decalComp] : _scene.GetRegistry().view<TransformComponent, DecalComponent>().each()) {
			Decal decalObj;
			decalObj.transform = transform.worldTransform;
			decalObj.inverseTransform = inverse(transform.worldTransform);

			uint32_t textureID = 0xFFFFFFFF;

			auto textureAsset = AssetManager::GetAsset<Texture2DAsset>(decalComp.texture);
			if (textureAsset) {
				if (auto it = _decalTextureIndexMap.find(textureAsset->GetTexture()); it != _decalTextureIndexMap.end()) {
					textureID = it->second;
				}
				else {
					const uint32_t offset = 3; // 0, 1, 2 are reserved for G-Buffer
					textureID = offset + _decalTextures.size();
					_decalTextureIndexMap[textureAsset->GetTexture()] = textureID;
					_decalTextures.push_back(textureAsset->GetTexture());
				}
			}

			decalObj.textureID = textureID;
			
			_decals.push_back(std::move(decalObj));
		}

		if (!_decals.empty()) {
			_decalSB->Update(_decals.data(), sizeof(Decal) * _decals.size());
		}
	}

	void RenderSystem::GatherRenderableObjects() {
		for (auto& [depth, stage] : _renderStages) {
			stage.renderQueue.Open();

			// submit mesh
			for (auto&& [entity, transform, staticMeshCom] : _scene.GetRegistry().view<TransformComponent, StaticMeshComponent>().each()) {
				auto meshAsset = AssetManager::GetAsset<StaticMeshAsset>(staticMeshCom.mesh);
				if (meshAsset == nullptr) {
					continue;
				}

				Ref<Mesh> mesh = meshAsset->GetMesh();

				// NOTE: test frustums with sphere, but in the future, may be need secondary frustum check for bounding cube.
				auto& boundingSphere = mesh->GetBoundingSphere();
				if (!stage.frustum.TestInside(boundingSphere.center, boundingSphere.radius, transform.worldTransform)) {
					continue;
				}

				for (int32_t i = 0; i < mesh->GetMeshSegmentCount(); ++i) {
					auto& materialHandle = staticMeshCom.materials[i];
					auto materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
					if (!materialAsset) {
						continue;
					}
					stage.renderQueue.Push(mesh, i, transform.worldTransform, materialAsset->GetMaterial());
				}
			}

			for (auto&& [entity, transform, skeletalMeshComp] : _scene.GetRegistry().view<TransformComponent, SkeletalMeshComponent>().each()) {
				auto meshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletalMeshComp.mesh);
				if (meshAsset == nullptr) {
					continue;
				}

				Ref<Mesh> mesh = meshAsset->GetMesh();

				// NOTE: test frustums with sphere, but in the future, may be need secondary frustum check for bounding cube.
				auto& boundingSphere = mesh->GetBoundingSphere();
				if (!stage.frustum.TestInside(boundingSphere.center, boundingSphere.radius, transform.worldTransform)) {
					continue;
				}

				auto& skeletalAnimData = _scene.GetAnimationSystem().GetSkeletalAnimationData(entity);
				auto boneMatrices = skeletalAnimData.GetBoneMatrices();
				if (boneMatrices == nullptr) {
					continue;
				}

				for (int32_t i = 0; i < mesh->GetMeshSegmentCount(); ++i) {
					auto& materialHandle = skeletalMeshComp.materials[i];

					auto materialAsset = AssetManager::GetAsset<MaterialAsset>(materialHandle);
					if (!materialAsset) {
						continue;
					}

					stage.renderQueue.Push(mesh, i, transform.worldTransform, materialAsset->GetMaterial(), boneMatrices);
				}
			}

			_scene.GetLandscapeSystem().GatherRenderable(stage);

			stage.renderQueue.Close();
		}
	}

	void RenderSystem::Render() {
		for (auto& [depth, stage] : _renderStages) {
			_cameraConstansCB.position = stage.cameraPosition;
			_cameraConstansCB.view = stage.viewMatrix;
			_cameraConstansCB.projection = stage.projectionMatrix;
			_vpCB->Update(&_cameraConstansCB, sizeof(CameraConstants));

			_scene.GetSkyBoxSystem().Render(_vpCB);
			_scene.GetShadowSystem().Render(stage, _batchedTransformSB);

			RenderGeometry(stage);
			RenderDecal(stage);
			RenderDefferdLighting(stage);
			RenderTransparent(stage);
			FinalizeRender(stage);
		}
	}

	void RenderSystem::UpdateMateraialConstants(GraphicsCommandQueue& cmdQueue, const Ref<Material>& material) {
		_materialConstants.reservedTextureBitMask = 0;
		if (material->albedoTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Albedo;
			cmdQueue.SetTexture(material->albedoTexture, ReservedTextureStartSlot);
		}
		if (material->normalTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Normal;
			cmdQueue.SetTexture(material->normalTexture, ReservedTextureStartSlot + 1);
		}
		if (material->emissiveTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Emissive;
			cmdQueue.SetTexture(material->emissiveTexture, ReservedTextureStartSlot + 2);
		}
		if (material->heightTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Height;
			cmdQueue.SetTexture(material->heightTexture, ReservedTextureStartSlot + 3);
		}
		if (material->metallicTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Metallic;
			cmdQueue.SetTexture(material->metallicTexture, ReservedTextureStartSlot + 4);
		}
		if (material->roughnessTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::Roughness;
			cmdQueue.SetTexture(material->roughnessTexture, ReservedTextureStartSlot + 5);
		}
		if (material->ambientOcclusionTexture) {
			_materialConstants.reservedTextureBitMask |= MaterialTextureType::AmbientOcclusion;
			cmdQueue.SetTexture(material->ambientOcclusionTexture, ReservedTextureStartSlot + 6);
		}
		for (int32_t i = 0; i < material->cubeTextures.size(); ++i) {
			if (material->cubeTextures[i]) {
				_materialConstants.cubeTextureBitMask |= (1 << i);
				cmdQueue.SetTexture(material->cubeTextures[i], CubeTextureStartSlot + i);
			}
		}
		for (int32_t i = 0; i < material->textureArrays.size(); ++i) {
			if (material->textureArrays[i]) {
				_materialConstants.textureArrayBitMask |= (1 << i);
				cmdQueue.SetTexture(material->textureArrays[i], TextureArrayStartSlot + i);
			}
		}

		std::memcpy(_materialConstants.intConstants, material->intConstants, sizeof(uint32_t) * 4 + sizeof(float) * 4 + sizeof(vec2) * 4 + sizeof(vec4) * 4);

		_materialCB->Update(&_materialConstants, sizeof(MaterialConstants));
	}

	void RenderSystem::RenderGeometry(CameraRenderStage& stage) {
		_geometryPass->Bind(true, false);

		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		while (!stage.renderQueue.Empty()) {
			auto& entry = stage.renderQueue.Front();
			if (!(entry.material->renderMode == RenderMode::Opaque || entry.material->renderMode == RenderMode::Masked)) {
				break;
			}

			// set pipeline
			pipeline->SetShader(entry.material->shader);
			pipeline->SetFillMode(FillMode::Solid);
			pipeline->SetCullMode(entry.material->cullMode);
			pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

			cmdQueue.SetPipeline(pipeline);
			cmdQueue.SetConstantBuffer(_vpCB, 0);
			cmdQueue.SetConstantBuffer(_globalCB, 1);
			cmdQueue.SetConstantBuffer(_lightCB, 2);
			UpdateMateraialConstants(cmdQueue, entry.material);
			cmdQueue.SetConstantBuffer(_materialCB, 3);
			cmdQueue.SetStructuredBuffer(_batchedTransformSB, 0);
			cmdQueue.Execute();

			// instancing draw
			for (auto& obj : entry.instancingObjects) {
				auto& mesh = obj.mesh;
				auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

				_batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

				cmdQueue.SetPrimitiveTopology(meshSegment.topology);
				cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
				cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);

				cmdQueue.Execute();
			}

			// skeletal mesh draw
			for (auto& obj : entry.skeletalInstancingObjects) {
				auto& mesh = obj.mesh;
				auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

				_batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

				cmdQueue.SetPrimitiveTopology(meshSegment.topology);
				cmdQueue.SetStructuredBuffer(obj.skeletonBoneMatrices, 1);
				cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
				cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
				cmdQueue.Execute();
			}

			stage.renderQueue.Pop();
		}

		_geometryPass->Unbind();
	}

	void RenderSystem::RenderDecal(CameraRenderStage& stage) {
		if (_decals.empty()) {
			return;
		}

		// NOTE: because of decal albedo render target referenced the geometry albedo render target texture, we should not clear clear it
		_decalPass->Bind(true, false);

		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		// TODO: Decal 시스템 추가하여 구현
		static bool init = false;
		static Ref<GraphicsShader> g_decalShader;
		static Ref<VertexBuffer> g_cubeVB;
		static Ref<IndexBuffer> g_cubeIB;

		if (!init) {
			g_decalShader = Graphics::CreateGraphicsShader("Resources/Shaders/decal3d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
			g_decalShader->AddInputElement<float>("POSITION", 3);
			g_decalShader->CreateInputLayout();

			std::vector<PointVertex> vertices;
			std::vector<uint32_t> indices;
			GenerateCube([&vertices](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
					PointVertex vertex = { pos };
					vertices.push_back(vertex);
				},
				indices
			);

			VertexBuffer::Descriptor vbDesc = {};
			vbDesc.elmSize = sizeof(PointVertex);
			vbDesc.bufferSize = sizeof(PointVertex) * vertices.size();
			vbDesc.usage = UsageFlag::Static;
			vbDesc.initialData = vertices.data();
			g_cubeVB = Graphics::CreateVertexBuffer(vbDesc);

			IndexBuffer::Descriptor ibDesc = {};
			ibDesc.bufferSize = sizeof(uint32_t) * indices.size();
			ibDesc.usage = UsageFlag::Static;
			ibDesc.initialData = indices.data();
			g_cubeIB = Graphics::CreateIndexBuffer(ibDesc);

			init = true;
		}

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		pipeline->SetShader(g_decalShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetStructuredBuffer(_decalSB, 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 2);
		for (int32_t i = 0; i < _decalTextures.size(); ++i) {
			cmdQueue.SetTexture(_decalTextures[i], 3 + i);
		}
		cmdQueue.SetVertexBuffer(g_cubeVB);
		cmdQueue.DrawIndexedInstanced(g_cubeIB, g_cubeIB->IndexCount(), _decals.size());

		cmdQueue.Execute();

		_decalPass->Unbind();
	}

	void RenderSystem::RenderDefferdLighting(CameraRenderStage& stage) {
		_lightingPass->Bind(true, false);

		auto& enttRegistry = _scene.GetRegistry();
		auto& shadowSys = _scene.GetShadowSystem();
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		//TODO: 라이팅 시스템 추가하여 구현할 것
		static bool init = false;
		static Ref<GraphicsShader> g_directionalLightShader;
		static Ref<VertexBuffer> g_fullscreenQuadVB;
		static Ref<IndexBuffer> g_fullscreenQuadIB;

		if (!init) {
			QuadVertex quadVertices[4] = {
				{ vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 0.0f) },
				{ vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 0.0f) },
				{ vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 1.0f) },
				{ vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 1.0f) },
			};

			uint32_t quadIndices[6] = {
				0, 1, 2,
				0, 2, 3
			};

			VertexBuffer::Descriptor vbDesc = {};
			vbDesc.elmSize = sizeof(QuadVertex);
			vbDesc.bufferSize = sizeof(QuadVertex) * 4;
			vbDesc.usage = UsageFlag::Static;
			vbDesc.initialData = quadVertices;
			g_fullscreenQuadVB = Graphics::CreateVertexBuffer(vbDesc);

			IndexBuffer::Descriptor ibDesc = {};
			ibDesc.bufferSize = sizeof(uint32_t) * 6;
			ibDesc.usage = UsageFlag::Static;
			ibDesc.initialData = quadIndices;
			g_fullscreenQuadIB = Graphics::CreateIndexBuffer(ibDesc);

			g_directionalLightShader = Graphics::CreateGraphicsShader("Resources/Shaders/lighting3d_directional.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
			g_directionalLightShader->AddInputElement<float>("POSITION", 3);
			g_directionalLightShader->AddInputElement<float>("TEXCOORD", 2);
			g_directionalLightShader->CreateInputLayout();

			init = true;
		}

		// render directional light
		pipeline->SetShader(g_directionalLightShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Back);
		pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetConstantBuffer(_directionalLightUniformCB, 3);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetVertexBuffer(g_fullscreenQuadVB);
		cmdQueue.Execute();

		for (auto&& [entity, transform, directionalLightComp] : enttRegistry.view<TransformComponent, DirectionalLightComponent>().each()) {
			auto& shadowMap = shadowSys.GetDirectionalLightShadowMap(entity);
			
			DirectionalLightUniforms uniforms = {};
			uniforms.view0 = shadowMap.lightVPMatrices[0].view;
			uniforms.projection0 = shadowMap.lightVPMatrices[0].projection;
			uniforms.cascadeDist0 = shadowMap.cascadeDistances[0];

			uniforms.view1 = shadowMap.lightVPMatrices[1].view;
			uniforms.projection1 = shadowMap.lightVPMatrices[1].projection;
			uniforms.cascadeDist1 = shadowMap.cascadeDistances[1];

			uniforms.view2 = shadowMap.lightVPMatrices[2].view;
			uniforms.projection2 = shadowMap.lightVPMatrices[2].projection;
			uniforms.cascadeDist2 = shadowMap.cascadeDistances[2];

			uniforms.lightColor = vec4(directionalLightComp.color, 1.0);
			uniforms.lightDirection = vec4(transform.GetWorldFront(), 0.0f);
			uniforms.lightIntensity = directionalLightComp.intensity;
			_directionalLightUniformCB->Update(&uniforms, sizeof(DirectionalLightUniforms));
	
			for (int32_t i = 0; i < 3; ++i) {
				cmdQueue.SetTexture(shadowMap.renderPasses[i]->GetRenderTargetTex(0), 4 + i);
			}

			cmdQueue.DrawIndexed(g_fullscreenQuadIB, g_fullscreenQuadIB->IndexCount());
			cmdQueue.Execute();
		}

		// render point light
		Ref<Mesh> sphereMesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_sphere_mesh")->GetMesh();
		auto pointLightShader = AssetManager::GetAsset<GraphicsShaderAsset>("lighting3d_point")->GetShader();

		pipeline->SetShader(pointLightShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetConstantBuffer(_pointLightUniformCB, 3);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetVertexBuffer(sphereMesh->GetGPUVertexBuffer());
		cmdQueue.Execute();

		for (auto&& [entity, transform, pointLightComp] : enttRegistry.view<TransformComponent, PointLightComponent>().each()) {
			auto& shadowMap = shadowSys.GetPointLightShadowMap(entity);

			PointLightUniforms uniforms = {};
			uniforms.lightColor = vec4(pointLightComp.color, 1.0);
			uniforms.lightPosition = vec4(transform.GetWorldPosition(), 1.0f);
			uniforms.lightIntensity = pointLightComp.intensity;
			uniforms.lightRange = pointLightComp.range;
			_pointLightUniformCB->Update(&uniforms, sizeof(PointLightUniforms));

			cmdQueue.SetTexture(shadowMap.renderPass->GetRenderTargetTex(0), 4);
			cmdQueue.DrawIndexed(sphereMesh->GetGPUIndexBuffer(), sphereMesh->GetGPUIndexBuffer()->IndexCount());
			cmdQueue.Execute();
		}

		// render spot light
		Ref<Mesh> coneMesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cone_mesh")->GetMesh();
		auto spotLightShader = AssetManager::GetAsset<GraphicsShaderAsset>("lighting3d_spot")->GetShader();

		pipeline->SetShader(spotLightShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetConstantBuffer(_spotLightUniformCB, 3);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetVertexBuffer(coneMesh->GetGPUVertexBuffer());
		cmdQueue.Execute();

		for (auto&& [entity, transform, spotLightComp] : enttRegistry.view<TransformComponent, SpotLightComponent>().each()) {
			auto& shadowmap = shadowSys.GetSpotLightShadowMap(entity);

			SpotLightUniforms uniforms = {};
			uniforms.view = shadowmap.lightVPMatrix.view;
			uniforms.projection = shadowmap.lightVPMatrix.projection;
			uniforms.lightColor = vec4(spotLightComp.color, 1.0);
			uniforms.lightDirection = vec4(transform.GetWorldFront(), 0.0f);
			uniforms.lightPosition = vec4(transform.GetWorldPosition(), 1.0f);
			uniforms.lightIntensity = spotLightComp.intensity;
			uniforms.innerAngle = spotLightComp.inner;
			uniforms.outerAngle = spotLightComp.outer;
			uniforms.lightRange = spotLightComp.range;
			_spotLightUniformCB->Update(&uniforms, sizeof(SpotLightUniforms));

			cmdQueue.SetTexture(shadowmap.renderPass->GetRenderTargetTex(0), 4);
			cmdQueue.DrawIndexed(coneMesh->GetGPUIndexBuffer(), coneMesh->GetGPUIndexBuffer()->IndexCount());
			cmdQueue.Execute();
		}

		_lightingPass->Unbind();
	}

	void RenderSystem::RenderTransparent(CameraRenderStage& stage) {
		// TODO: forward rendering
		while (!stage.renderQueue.Empty()) {
			auto& entry = stage.renderQueue.Front();
			if (!(entry.material->renderMode == RenderMode::Transparent)) {
				break;
			}	

			// TODO: render to main render target

			stage.renderQueue.Pop();
		}
	}

	void RenderSystem::FinalizeRender(CameraRenderStage& stage) {
		auto& mainPass = Graphics::GetMainRenderPass();
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		mainPass->SetBlendMode(0, BlendMode::Alpha, false);
		mainPass->Bind(false, false);
	
		// TODO: temp
		static bool init = false;
		static Ref<GraphicsShader> g_std3dFinalizeShader;
		static Ref<VertexBuffer> g_fullscreenQuadVB;
		static Ref<IndexBuffer> g_fullscreenQuadIB;

		if (!init) {
			g_std3dFinalizeShader = Graphics::CreateGraphicsShader("Resources/Shaders/std3d_finalize.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
			g_std3dFinalizeShader->AddInputElement<float>("POSITION", 3);
			g_std3dFinalizeShader->AddInputElement<float>("TEXCOORD", 2);
			g_std3dFinalizeShader->CreateInputLayout();

			QuadVertex quadVertices[4] = {
				{ vec3(-1.0f, 1.0f, 0.0f), vec2(0.0f, 0.0f) },
				{ vec3(1.0f, 1.0f, 0.0f), vec2(1.0f, 0.0f) },
				{ vec3(1.0f, -1.0f, 0.0f), vec2(1.0f, 1.0f) },
				{ vec3(-1.0f, -1.0f, 0.0f), vec2(0.0f, 1.0f) },
			};

			uint32_t quadIndices[6] = {
				0, 1, 2,
				0, 2, 3
			};

			VertexBuffer::Descriptor vbDesc = {};
			vbDesc.elmSize = sizeof(QuadVertex);
			vbDesc.bufferSize = sizeof(QuadVertex) * 4;
			vbDesc.usage = UsageFlag::Static;
			vbDesc.initialData = quadVertices;
			g_fullscreenQuadVB = Graphics::CreateVertexBuffer(vbDesc);

			IndexBuffer::Descriptor ibDesc = {};
			ibDesc.bufferSize = sizeof(uint32_t) * 6;
			ibDesc.usage = UsageFlag::Static;
			ibDesc.initialData = quadIndices;
			g_fullscreenQuadIB = Graphics::CreateIndexBuffer(ibDesc);

			init = true;
		}

		pipeline->SetShader(g_std3dFinalizeShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Back);
		pipeline->SetDepthTest(DepthTest::Always, false);

		SkyBox& skybox = _scene.GetSkyBoxSystem().GetSkyBox();

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryEmissive), 4);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingRadiance), 5);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingShadow), 6);
		if (skybox.originalTexture) {
			cmdQueue.SetTexture(skybox.irradianceTexture, 7);
			cmdQueue.SetTexture(skybox.prefilteredTexture, 8);
			cmdQueue.SetTexture(skybox.brdfLUTTexture, 9);
		}
		cmdQueue.SetVertexBuffer(g_fullscreenQuadVB);
		cmdQueue.DrawIndexed(g_fullscreenQuadIB, g_fullscreenQuadIB->IndexCount());

		cmdQueue.Execute();

		// TODO: test
#if true
		auto& enttRegistry = _scene.GetRegistry();

		Renderer2D::Begin(stage.viewMatrix, stage.projectionMatrix);

		for (auto&& [entity, transComp, sprComp] : enttRegistry.view<TransformComponent, SpriteRendererComponent>().each()) {
			auto textureAsset = AssetManager::GetAsset<Texture2DAsset>(sprComp.texture);
			if (!textureAsset) {
				Renderer2D::DrawQuad(entity, transComp.worldTransform, sprComp.color);
			}
			else {
				Renderer2D::DrawQuad(entity, transComp.worldTransform, textureAsset->GetTexture());
			}
		}

		for (auto&& [entity, transComp, textComp] : enttRegistry.view<TransformComponent, TextComponent>().each()) {
			auto fontAsset = AssetManager::GetAsset<FontAsset>(textComp.font);
			if (fontAsset) {
				Renderer2D::DrawString(entity, transComp.worldTransform, textComp.text, fontAsset->GetFont(), fontAsset->GetFontAtlas(), textComp.color);
			}
		}
		Renderer2D::End();
#endif

		_scene.GetParticleSystem().Render(_vpCB, _globalCB);
	}
}