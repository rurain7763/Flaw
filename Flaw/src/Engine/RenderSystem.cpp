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

// TODO: remove this
#include "Renderer2D.h"
#include "SkyBoxSystem.h"
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
		objMRTDesc.renderTargets[GeometryPosition].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryNormal].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryNormal].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryNormal].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryAlbedo].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryAlbedo].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryAlbedo].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryMaterial].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryMaterial].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryMaterial].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		objMRTDesc.renderTargets[GeometryEmissive].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryEmissive].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryEmissive].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		objMRTDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		objMRTDesc.depthStencil.resizeFunc = [](Ref<Texture2D>& current, int32_t width, int32_t height) {
			return Graphics::GetMainRenderPass()->GetDepthStencilTex();
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
		decalPassDesc.renderTargets[DecalAlbedo].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };
		decalPassDesc.renderTargets[DecalAlbedo].resizeFunc = [this](Ref<Texture2D>& current, int32_t width, int32_t height) {
			return _geometryPass->GetRenderTargetTex(GeometryAlbedo);
		};

		decalPassDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		decalPassDesc.depthStencil.resizeFunc = [](Ref<Texture2D>& current, int32_t width, int32_t height) {
			return Graphics::GetMainRenderPass()->GetDepthStencilTex();
		};

		_decalPass = Graphics::CreateRenderPass(decalPassDesc);

		// light MRT
		GraphicsRenderPass::Descriptor lightMRTDesc = {};
		lightMRTDesc.renderTargets.resize(LightingRTCount);

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingRadiance].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingRadiance].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingRadiance].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingShadow].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingShadow].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingShadow].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		lightMRTDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		lightMRTDesc.depthStencil.resizeFunc = [](Ref<Texture2D>& current, int32_t width, int32_t height) {
			return Graphics::GetMainRenderPass()->GetDepthStencilTex();
		};

		_lightingPass = Graphics::CreateRenderPass(lightMRTDesc);
	}

	void RenderSystem::CreateConstantBuffers() {
		_vpCB = Graphics::CreateConstantBuffer(sizeof(CameraConstants));
		_globalCB = Graphics::CreateConstantBuffer(sizeof(GlobalConstants));
		_lightCB = Graphics::CreateConstantBuffer(sizeof(LightConstants));
		_materialCB = Graphics::CreateConstantBuffer(sizeof(MaterialConstants));
		_directionalLightUniformCB = Graphics::CreateConstantBuffer(sizeof(DirectionalLightUniforms));
	}

	void RenderSystem::CreateStructuredBuffers() {
		// Create a structured buffer for batched transforms
		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(mat4);
		sbDesc.count = MaxBatchTransformCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		_batchedTransformSB = Graphics::CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for point lights
		sbDesc.elmSize = sizeof(PointLight);
		sbDesc.count = MaxPointLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		_pointLightSB = Graphics::CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for spot lights
		sbDesc.elmSize = sizeof(SpotLight);
		sbDesc.count = MaxSpotLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		_spotLightSB = Graphics::CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for decals
		sbDesc.elmSize = sizeof(Decal);
		sbDesc.count = MaxDecalCount;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		_decalSB = Graphics::CreateStructuredBuffer(sbDesc);
	}

	void RenderSystem::Update() {
		std::map<uint32_t, Camera> cameras;

		for (auto&& [entity, transformComp, cameraComp] : _scene.GetRegistry().view<TransformComponent, CameraComponent>().each()) {
			Camera camera;
			camera.isPerspective = cameraComp.perspective;
			camera.position = transformComp.GetWorldPosition();
			camera.view = ViewMatrix(transformComp.position, transformComp.rotation);
			camera.projection = cameraComp.GetProjectionMatrix();

			if (cameraComp.perspective) {
				CreateFrustrum(
					GetFovX(cameraComp.fov, cameraComp.aspectRatio), 
					cameraComp.fov, 
					cameraComp.nearClip, 
					cameraComp.farClip, 
					transformComp.worldTransform, 
					camera.frustrum
				);
			}
			else {
				// Orthographic
			}

			cameras[cameraComp.depth] = camera;
		}

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
		GatherCameraStages(cameras);
	}

	void RenderSystem::Update(Camera& camera) {
		std::map<uint32_t, Camera> cameras;
		cameras[0] = camera;

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
		GatherCameraStages(cameras);
	}

	void RenderSystem::GatherLights() {
		auto& enttRegistry = _scene.GetRegistry();
		auto& shadowSystem = _scene.GetShadowSystem();

		_lightConstants = {};
		_pointLights.clear();
		_spotLights.clear();

		for (auto&& [entity, transform, skyLightComp] : enttRegistry.view<TransformComponent, SkyLightComponent>().each()) {
			// Skylight's 하나만 존재해야 함
			_lightConstants.ambientColor = skyLightComp.color;
			_lightConstants.ambientIntensity = skyLightComp.intensity;
			break;
		}

		for (auto&& [entity, transform, pointLightComp] : enttRegistry.view<TransformComponent, PointLightComponent>().each()) {
			_pointLights.emplace_back(PointLight{
				pointLightComp.color,
				pointLightComp.intensity,
				transform.GetWorldPosition(),
				pointLightComp.range
			});
		}

		for (auto&& [entity, transform, spotLight] : enttRegistry.view<TransformComponent, SpotLightComponent>().each()) {
			SpotLight light;
			light.color = spotLight.color;
			light.intensity = spotLight.intensity;
			light.position = transform.GetWorldPosition();
			light.direction = transform.GetWorldFront();
			light.inner = spotLight.inner;
			light.outer = spotLight.outer;
			light.range = spotLight.range;
			_spotLights.push_back(std::move(light));
		}

		_lightConstants.numPointLights = std::min((uint32_t)_pointLights.size(), MaxPointLights);
		_lightConstants.numSpotLights = std::min((uint32_t)_spotLights.size(), MaxSpotLights);
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

	void RenderSystem::GatherCameraStages(std::map<uint32_t, Camera>& cameras) {
		_renderStages.resize(cameras.size());
		for (const auto& [depth, camera] : cameras) {
			auto& stage = _renderStages[depth];

			stage.cameraPosition = camera.position;
			stage.view = camera.view;
			stage.projection = camera.projection;		

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
				if (camera.isPerspective && !camera.TestInFrustum(boundingSphere.center, boundingSphere.radius, transform.worldTransform)) {
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
				if (camera.isPerspective && !camera.TestInFrustum(boundingSphere.center, boundingSphere.radius, transform.worldTransform)) {
					continue;
				}

				auto& skeletalAnimData = _scene.GetAnimationSystem().GetSkeletalAnimationData((uint32_t)entity);
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

			_scene.GetLandscapeSystem().Render(camera, stage.renderQueue);

			stage.renderQueue.Close();
		}
	}

	void RenderSystem::Render() {
		for (auto& stage : _renderStages) {
			_cameraConstansCB.position = stage.cameraPosition;
			_cameraConstansCB.view = stage.view;
			_cameraConstansCB.projection = stage.projection;
			_vpCB->Update(&_cameraConstansCB, sizeof(CameraConstants));

			_scene.GetSkyBoxSystem().Render(_vpCB, _globalCB, _lightCB, _materialCB);
			_scene.GetShadowSystem().Render(_batchedTransformSB);

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
			auto& shadowMap = shadowSys.GetShadowMap((uint32_t)entity);
			
			DirectionalLightUniforms uniforms = {};
			uniforms.view = shadowMap.uniforms.view;
			uniforms.projection = shadowMap.uniforms.projection;
			uniforms.lightColor = vec4(directionalLightComp.color, 1.0);
			uniforms.lightDirection = vec4(transform.GetWorldFront(), 0.0f);
			uniforms.lightIntensity = directionalLightComp.intensity;

			_directionalLightUniformCB->Update(&uniforms, sizeof(DirectionalLightUniforms));
	
			cmdQueue.SetTexture(shadowMap.renderPass->GetRenderTargetTex(0), 4);
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

		_pointLightSB->Update(_pointLights.data(), sizeof(PointLight) * _lightConstants.numPointLights);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetStructuredBuffer(_pointLightSB, 5);
		cmdQueue.SetVertexBuffer(sphereMesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexedInstanced(sphereMesh->GetGPUIndexBuffer(), sphereMesh->GetGPUIndexBuffer()->IndexCount(), _lightConstants.numPointLights);

		cmdQueue.Execute();

		// render spot light
		Ref<Mesh> coneMesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cone_mesh")->GetMesh();
		auto spotLightShader = AssetManager::GetAsset<GraphicsShaderAsset>("lighting3d_spot")->GetShader();

		pipeline->SetShader(spotLightShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled, false);

		_spotLightSB->Update(_spotLights.data(), sizeof(SpotLight)* _lightConstants.numSpotLights);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 3);
		cmdQueue.SetStructuredBuffer(_spotLightSB, 5);
		cmdQueue.SetVertexBuffer(coneMesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexedInstanced(coneMesh->GetGPUIndexBuffer(), coneMesh->GetGPUIndexBuffer()->IndexCount(), _lightConstants.numSpotLights);

		cmdQueue.Execute();

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

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryMaterial), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryEmissive), 2);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingRadiance), 3);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingShadow), 4);
		cmdQueue.SetVertexBuffer(g_fullscreenQuadVB);
		cmdQueue.DrawIndexed(g_fullscreenQuadIB, g_fullscreenQuadIB->IndexCount());

		cmdQueue.Execute();

		// TODO: test
#if true
		auto& enttRegistry = _scene.GetRegistry();

		Renderer2D::Begin(stage.view, stage.projection);

		for (auto&& [entity, transComp, sprComp] : enttRegistry.view<TransformComponent, SpriteRendererComponent>().each()) {
			auto textureAsset = AssetManager::GetAsset<Texture2DAsset>(sprComp.texture);
			if (!textureAsset) {
				Renderer2D::DrawQuad((uint32_t)entity, transComp.worldTransform, sprComp.color);
			}
			else {
				Renderer2D::DrawQuad((uint32_t)entity, transComp.worldTransform, textureAsset->GetTexture());
			}
		}

		for (auto&& [entity, transComp, textComp] : enttRegistry.view<TransformComponent, TextComponent>().each()) {
			auto fontAsset = AssetManager::GetAsset<FontAsset>(textComp.font);
			if (fontAsset) {
				Renderer2D::DrawString((uint32_t)entity, transComp.worldTransform, textComp.text, fontAsset->GetFont(), fontAsset->GetFontAtlas(), textComp.color);
			}
		}
		Renderer2D::End();
#endif

		_scene.GetParticleSystem().Render(_vpCB, _globalCB);
	}
}