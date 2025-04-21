#include "pch.h"
#include "RenderSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Time/Time.h"
#include "Graphics/GraphicsFunc.h"
#include "LandscapeSystem.h"

// TODO: remove this
#include "Image/Image.h"
#include "Renderer2D.h"
#include "Renderer.h"
#include "SkyBoxSystem.h"
#include "ParticleSystem.h"

namespace flaw {
	RenderSystem::RenderSystem(Scene& scene)
		: _scene(scene)
	{
		CreateRenderPasses();
		CreateBatchedBuffers();
		CreateConstantBuffers();
		CreateStructuredBuffers();
		CreatePipeline();
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
		objMRTDesc.renderTargets[GeometryEmissive].blendMode = BlendMode::Disabled;
		objMRTDesc.renderTargets[GeometryEmissive].texture = Graphics::CreateTexture2D(texDesc);
		objMRTDesc.renderTargets[GeometryEmissive].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		objMRTDesc.depthStencil.texture = Graphics::GetMainRenderPass()->GetDepthStencilTex();
		objMRTDesc.depthStencil.resizeFunc = [](int32_t width, int32_t height) {
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
		decalPassDesc.renderTargets[DecalAlbedo].resizeFunc = [this](int32_t width, int32_t height) {
			return _geometryPass->GetRenderTargetTex(GeometryAlbedo);
		};

		_decalPass = Graphics::CreateRenderPass(decalPassDesc);

		// light MRT
		GraphicsRenderPass::Descriptor lightMRTDesc = {};
		lightMRTDesc.renderTargets.resize(LightingRTCount);

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingDiffuse].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingDiffuse].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingDiffuse].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::RGBA32F;
		lightMRTDesc.renderTargets[LightingSpecular].blendMode = BlendMode::Additive;
		lightMRTDesc.renderTargets[LightingSpecular].texture = Graphics::CreateTexture2D(texDesc);
		lightMRTDesc.renderTargets[LightingSpecular].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		_lightingPass = Graphics::CreateRenderPass(lightMRTDesc);
	}

	void RenderSystem::CreateBatchedBuffers() {
		// Create a vertex buffer for batched geometry
		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.elmSize = sizeof(Vertex3D);
		vbDesc.bufferSize = sizeof(Vertex3D) * MaxBatchVertexCount;
		vbDesc.usage = UsageFlag::Dynamic;

		_batchedVertexBuffer = Graphics::CreateVertexBuffer(vbDesc);

		// Create an index buffer for batched geometry
		IndexBuffer::Descriptor ibDesc = {};
		ibDesc.bufferSize = sizeof(uint32_t) * MaxBatchIndexCount;
		ibDesc.usage = UsageFlag::Dynamic;

		_batchedIndexBuffer = Graphics::CreateIndexBuffer(ibDesc);
	}

	void RenderSystem::CreateConstantBuffers() {
		_vpCB = Graphics::CreateConstantBuffer(sizeof(VPMatrices));
		_globalCB = Graphics::CreateConstantBuffer(sizeof(GlobalConstants));
		_lightCB = Graphics::CreateConstantBuffer(sizeof(LightConstants));
		_materialCB = Graphics::CreateConstantBuffer(sizeof(MaterialConstants));
	}

	void RenderSystem::CreateStructuredBuffers() {
		// Create a structured buffer for batched transforms
		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(mat4);
		sbDesc.count = MaxBatchTransformCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		_batchedTransformSB = Graphics::CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for directional lights
		sbDesc.elmSize = sizeof(DirectionalLight);
		sbDesc.count = MaxDirectionalLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		_directionalLightSB = Graphics::CreateStructuredBuffer(sbDesc);

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

	void RenderSystem::CreatePipeline() {
		_pipeline = Graphics::CreateGraphicsPipeline();
	}

	void RenderSystem::Update() {
		std::map<uint32_t, Camera> cameras;

		for (auto&& [entity, transformComp, cameraComp] : _scene.GetRegistry().view<TransformComponent, CameraComponent>().each()) {
			Camera camera;
			camera.isPerspective = cameraComp.perspective;
			camera.view = ViewMatrix(transformComp.position, transformComp.rotation);
			camera.projection = cameraComp.GetProjectionMatrix();

			if (cameraComp.perspective) {
				CreateFrustrum(GetFovX(cameraComp.fov, cameraComp.aspectRatio), cameraComp.fov, cameraComp.nearClip, cameraComp.farClip, transformComp.worldTransform, camera.frustrum);
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

		_scene.GetParticleSystem().Update(_globalCB);

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

		_scene.GetParticleSystem().Update(_globalCB);

		GatherLights();
		GatherDecals();
		GatherCameraStages(cameras);
	}

	void RenderSystem::GatherLights() {
		auto& enttRegistry = _scene.GetRegistry();

		_lightConstants = {};
		_directionalLights.clear();
		_pointLights.clear();
		_spotLights.clear();

		for (auto&& [entity, transform, skyLightComp] : enttRegistry.view<TransformComponent, SkyLightComponent>().each()) {
			// Skylight's 하나만 존재해야 함
			_lightConstants.ambientColor = skyLightComp.color;
			_lightConstants.ambientIntensity = skyLightComp.intensity;
			break;
		}

		for (auto&& [entity, transform, directionalLightComp] : enttRegistry.view<TransformComponent, DirectionalLightComponent>().each()) {
			DirectionalLight light;
			light.color = directionalLightComp.color;
			light.intensity = directionalLightComp.intensity;
			light.direction = transform.GetWorldFront();
			_directionalLights.push_back(std::move(light));
		}

		for (auto&& [entity, transform, pointLight] : enttRegistry.view<TransformComponent, PointLightComponent>().each()) {
			PointLight light;
			light.color = pointLight.color;
			light.intensity = pointLight.intensity;
			light.position = transform.GetWorldPosition();
			light.range = pointLight.range;
			_pointLights.push_back(std::move(light));
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

		_lightConstants.numDirectionalLights = std::min((uint32_t)_directionalLights.size(), MaxDirectionalLights);
		_lightConstants.numPointLights = std::min((uint32_t)_pointLights.size(), MaxPointLights);
		_lightConstants.numSpotLights = std::min((uint32_t)_spotLights.size(), MaxSpotLights);
		_lightCB->Update(&_lightConstants, sizeof(LightConstants));

		_directionalLightSB->Update(_directionalLights.data(), sizeof(DirectionalLight) * _lightConstants.numDirectionalLights);
		_pointLightSB->Update(_pointLights.data(), sizeof(PointLight) * _lightConstants.numPointLights);
		_spotLightSB->Update(_spotLights.data(), sizeof(SpotLight) * _lightConstants.numSpotLights);
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

			stage.view = camera.view;
			stage.projection = camera.projection;		

			stage.renderQueue.Open();

			// submit mesh
			for (auto&& [entity, transform, meshFilter, meshRenderer] : _scene.GetRegistry().view<TransformComponent, MeshFilterComponent, MeshRendererComponent>().each()) {
#if false
#else
				// TODO: 메쉬 그리기 현재는 하드코딩된 메쉬만 그려짐
				static Ref<Mesh> g_sphereMesh;
				static Ref<GraphicsShader> g_std3dShader;
				static Ref<Material> g_material;
				
				if (g_sphereMesh == nullptr) {
					g_sphereMesh = CreateRef<Mesh>();

					GenerateSphere([](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
							Vertex3D vertex;
							vertex.position = pos;
							vertex.texcoord = uv;
							vertex.normal = normal;
							vertex.tangent = tangent;
							vertex.binormal = binormal;
							g_sphereMesh->vertices.push_back(vertex);
						},
						g_sphereMesh->indices, 20, 20
					);

					std::vector<vec3> vertices;
					std::transform(g_sphereMesh->vertices.begin(), g_sphereMesh->vertices.end(), std::back_inserter(vertices), [](const Vertex3D& vertex) { return vertex.position; });
					CreateBoundingSphere(vertices, g_sphereMesh->boundingSphereCenter, g_sphereMesh->boundingSphereRadius);
				}

				if (g_std3dShader == nullptr) {
					g_std3dShader = Graphics::CreateGraphicsShader("Resources/Shaders/std3d_geometry.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
					g_std3dShader->AddInputElement<float>("POSITION", 3);
					g_std3dShader->AddInputElement<float>("TEXCOORD", 2);
					g_std3dShader->AddInputElement<float>("TANGENT", 3);
					g_std3dShader->AddInputElement<float>("NORMAL", 3);
					g_std3dShader->AddInputElement<float>("BINORMAL", 3);
					g_std3dShader->CreateInputLayout();
				}

				if (g_material == nullptr) {
					g_material = CreateRef<Material>();
					g_material->shader = g_std3dShader;
					g_material->renderMode = RenderMode::Opaque;
					g_material->cullMode = CullMode::Back;
					g_material->depthTest = DepthTest::Less;
					g_material->depthWrite = true;

					Image albedo("Resources/PavingStone_albedo.jpg", 4);

					Texture2D::Descriptor desc = {};
					desc.width = albedo.Width();
					desc.height = albedo.Height();
					desc.format = PixelFormat::RGBA8;
					desc.data = albedo.Data().data();
					desc.usage = UsageFlag::Static;
					desc.bindFlags = BindFlag::ShaderResource;

					g_material->albedoTexture = Graphics::CreateTexture2D(desc);

					Image normal("Resources/PavingStone_normal.jpg", 4);

					desc = {};
					desc.width = normal.Width();
					desc.height = normal.Height();
					desc.format = PixelFormat::RGBA8;
					desc.data = normal.Data().data();
					desc.usage = UsageFlag::Static;
					desc.bindFlags = BindFlag::ShaderResource;

					g_material->normalTexture = Graphics::CreateTexture2D(desc);
				}

				// NOTE: test frustums with sphere, but in the future, may be need secondary frustum check for bounding cube.
				if (camera.isPerspective && camera.TestInFrustum(g_sphereMesh->boundingSphereCenter, g_sphereMesh->boundingSphereRadius, transform.worldTransform)) {
					stage.renderQueue.Push(g_sphereMesh, transform.worldTransform, g_material);
				}
#endif
			}

			_scene.GetLandscapeSystem().Render(camera, stage.renderQueue);

			stage.renderQueue.Close();
		}
	}

	void RenderSystem::Render() {
		for (auto& stage : _renderStages) {
			_vpMatrices.view = stage.view;
			_vpMatrices.projection = stage.projection;
			_vpCB->Update(&_vpMatrices, sizeof(VPMatrices));

			RenderGeometry(stage);
			RenderDecal(stage);
			RenderDefferdLighting(stage);
			RenderTransparent(stage);
			RenderSkyBox(stage);
			// TODO: Add more render stages
			FinalizeRender(stage);
		}
	}

	void RenderSystem::RenderGeometry(CameraRenderStage& stage) {
		_geometryPass->Bind();

		auto& cmdQueue = Graphics::GetCommandQueue();

		while (!stage.renderQueue.Empty()) {
			auto& entry = stage.renderQueue.Front();
			if (!(entry.material->renderMode == RenderMode::Opaque || entry.material->renderMode == RenderMode::Masked)) {
				break;
			}

			cmdQueue.Begin();

			// set pipeline
			_pipeline->SetShader(entry.material->shader);
			_pipeline->SetCullMode(entry.material->cullMode);
			_pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

			cmdQueue.SetPipeline(_pipeline);

			cmdQueue.SetConstantBuffer(_vpCB, 0);
			cmdQueue.SetConstantBuffer(_globalCB, 1);
			cmdQueue.SetConstantBuffer(_lightCB, 2);

			// set material properties
			_materialConstants.reservedTextureBitMask = 0;
			if (entry.material->albedoTexture) {
				_materialConstants.reservedTextureBitMask |= MaterialConstants::Albedo;
				cmdQueue.SetTexture(entry.material->albedoTexture, ReservedTextureStartSlot);
			}

			if (entry.material->normalTexture) {
				_materialConstants.reservedTextureBitMask |= MaterialConstants::Normal;
				cmdQueue.SetTexture(entry.material->normalTexture, ReservedTextureStartSlot + 1);
			}

			if (entry.material->emissiveTexture) {
				_materialConstants.reservedTextureBitMask |= MaterialConstants::Emissive;
				cmdQueue.SetTexture(entry.material->emissiveTexture, ReservedTextureStartSlot + 2);
			}

			if (entry.material->heightTexture) {
				_materialConstants.reservedTextureBitMask |= MaterialConstants::Height;
				cmdQueue.SetTexture(entry.material->heightTexture, ReservedTextureStartSlot + 3);
			}

			_materialConstants.cubeTextureBitMask = 0;
			for (int32_t i = 0; i < entry.material->cubeTextures.size(); ++i) {
				if (entry.material->cubeTextures[i]) {
					_materialConstants.cubeTextureBitMask |= (1 << i);
					cmdQueue.SetTexture(entry.material->cubeTextures[i], CubeTextureStartSlot + i);
				}
			}

			std::memcpy(
				_materialConstants.intConstants,
				entry.material->intConstants,
				sizeof(uint32_t) * 4 + sizeof(float) * 4
			);

			_materialCB->Update(&_materialConstants, sizeof(MaterialConstants));

			cmdQueue.SetConstantBuffer(_materialCB, 3);

			cmdQueue.SetStructuredBuffer(_batchedTransformSB, 0);
			cmdQueue.End();

			cmdQueue.Execute();

			// instancing draw
			for (auto& instancingObj : entry.instancingObjects) {
				auto& mesh = instancingObj.first;
				auto& instance = instancingObj.second;

				_batchedVertexBuffer->Update(mesh->vertices.data(), sizeof(Vertex3D), mesh->vertices.size());
				_batchedIndexBuffer->Update(mesh->indices.data(), mesh->indices.size());
				_batchedTransformSB->Update(instance.modelMatrices.data(), instance.modelMatrices.size() * sizeof(mat4));

				cmdQueue.Begin();
				cmdQueue.SetPrimitiveTopology(mesh->topology);
				cmdQueue.SetVertexBuffer(_batchedVertexBuffer);
				cmdQueue.DrawIndexedInstanced(_batchedIndexBuffer, mesh->indices.size(), instance.instanceCount);
				cmdQueue.End();

				cmdQueue.Execute();
			}

			// non-instancing draw
			for (auto& noBatchObj : entry.noBatchedObjects) {
				auto& mesh = noBatchObj.first;
				auto& modelMatrix = noBatchObj.second;

				_batchedVertexBuffer->Update(mesh->vertices.data(), sizeof(Vertex3D), mesh->vertices.size());
				_batchedIndexBuffer->Update(mesh->indices.data(), mesh->indices.size());
				_batchedTransformSB->Update(&modelMatrix, sizeof(mat4));

				cmdQueue.Begin();
				cmdQueue.SetPrimitiveTopology(mesh->topology);
				cmdQueue.SetVertexBuffer(_batchedVertexBuffer);
				cmdQueue.DrawIndexedInstanced(_batchedIndexBuffer, mesh->indices.size(), 1);
				cmdQueue.End();

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
		_decalPass->Bind(false);

		auto& cmdQueue = Graphics::GetCommandQueue();

		// TODO: temp
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

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		_pipeline->SetShader(g_decalShader);
		_pipeline->SetCullMode(CullMode::Front);
		_pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(_pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetStructuredBuffer(_decalSB, 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 2);
		cmdQueue.SetTextures((const Ref<Texture>*)_decalTextures.data(), _decalTextures.size(), 3);
		cmdQueue.SetVertexBuffer(g_cubeVB);
		cmdQueue.DrawIndexedInstanced(g_cubeIB, g_cubeIB->IndexCount(), _decals.size());
		cmdQueue.End();

		cmdQueue.Execute();

		_decalPass->Unbind();
	}

	void RenderSystem::RenderDefferdLighting(CameraRenderStage& stage) {
		_lightingPass->Bind();

		auto& cmdQueue = Graphics::GetCommandQueue();

		//TODO: temp
		static bool init = false;
		static Ref<GraphicsShader> g_directionalLightShader;
		static Ref<VertexBuffer> g_fullscreenQuadVB;
		static Ref<IndexBuffer> g_fullscreenQuadIB;
		static Ref<GraphicsShader> g_pointLightShader;
		static Ref<VertexBuffer> g_sphereVB;
		static Ref<IndexBuffer> g_sphereIB;

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

			// sphere
			std::vector<Vertex3D> sphereVertices;
			std::vector<uint32_t> sphereIndices;
			GenerateSphere([&sphereVertices](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
					Vertex3D vertex;
					vertex.position = pos;
					vertex.texcoord = uv;
					vertex.normal = normal;
					vertex.tangent = tangent;
					vertex.binormal = binormal;
					sphereVertices.push_back(vertex);
				},
				sphereIndices, 20, 20
			);
			
			vbDesc = {};
			vbDesc.usage = UsageFlag::Static;
			vbDesc.elmSize = sizeof(Vertex3D);
			vbDesc.bufferSize = sizeof(Vertex3D) * sphereVertices.size();
			vbDesc.initialData = sphereVertices.data();
			g_sphereVB = Graphics::CreateVertexBuffer(vbDesc);

			ibDesc = {};
			ibDesc.usage = UsageFlag::Static;
			ibDesc.bufferSize = sizeof(uint32_t) * sphereIndices.size();
			ibDesc.initialData = sphereIndices.data();
			g_sphereIB = Graphics::CreateIndexBuffer(ibDesc);

			g_pointLightShader = Graphics::CreateGraphicsShader("Resources/Shaders/lighting3d_point.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
			g_pointLightShader->AddInputElement<float>("POSITION", 3);
			g_pointLightShader->CreateInputLayout();

			// cone
			
			init = true;
		}

		// render directional light
		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
	
		_pipeline->SetShader(g_directionalLightShader);
		_pipeline->SetCullMode(CullMode::Back);
		_pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(_pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetStructuredBuffer(_directionalLightSB, 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 2);
		cmdQueue.SetVertexBuffer(g_fullscreenQuadVB);
		cmdQueue.DrawIndexed(g_fullscreenQuadIB, g_fullscreenQuadIB->IndexCount());
		cmdQueue.End();

		cmdQueue.Execute();
	
		// render point light
		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		_pipeline->SetShader(g_pointLightShader);
		_pipeline->SetCullMode(CullMode::Front);
		_pipeline->SetDepthTest(DepthTest::Disabled, false);

		cmdQueue.SetPipeline(_pipeline);
		cmdQueue.SetConstantBuffer(_vpCB, 0);
		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetStructuredBuffer(_pointLightSB, 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryPosition), 1);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryNormal), 2);
		cmdQueue.SetVertexBuffer(g_sphereVB);
		cmdQueue.DrawIndexedInstanced(g_sphereIB, g_sphereIB->IndexCount(), _lightConstants.numPointLights);
		cmdQueue.End();

		cmdQueue.Execute();

		// TODO: render spot light

		_lightingPass->Unbind();
	}

	void RenderSystem::RenderSkyBox(CameraRenderStage& stage) {
		auto& skyboxSys = _scene.GetSkyBoxSystem();

		RenderEnvironment renderEnv;
		renderEnv.view = stage.view;
		renderEnv.projection = stage.projection;

		Renderer::Begin(renderEnv);
		skyboxSys.Render();
		Renderer::End();
	}

	void RenderSystem::RenderTransparent(CameraRenderStage& stage) {
		// forward rendering
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
		auto& cmdQueue = Graphics::GetCommandQueue();
	
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

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		_pipeline->SetShader(g_std3dFinalizeShader);
		_pipeline->SetCullMode(CullMode::Back);
		_pipeline->SetDepthTest(DepthTest::Always, false);

		cmdQueue.SetPipeline(_pipeline);

		cmdQueue.SetConstantBuffer(_globalCB, 1);
		cmdQueue.SetConstantBuffer(_lightCB, 2);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryAlbedo), 0);
		cmdQueue.SetTexture(_geometryPass->GetRenderTargetTex(GeometryEmissive), 1);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingDiffuse), 2);
		cmdQueue.SetTexture(_lightingPass->GetRenderTargetTex(LightingSpecular), 3);
		cmdQueue.SetVertexBuffer(g_fullscreenQuadVB);
		cmdQueue.DrawIndexed(g_fullscreenQuadIB, g_fullscreenQuadIB->IndexCount());
		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();

		auto& enttRegistry = _scene.GetRegistry();

		Renderer2D::Begin(stage.view, stage.projection);
#if true
		for (auto&& [entity, transComp, sprComp] : enttRegistry.view<TransformComponent, SpriteRendererComponent>().each()) {
			auto textureAsset = AssetManager::GetAsset<Texture2DAsset>(sprComp.texture);
			if (!textureAsset) {
				Renderer2D::DrawQuad((uint32_t)entity, transComp.worldTransform, sprComp.color);
			}
			else {
				Renderer2D::DrawQuad((uint32_t)entity, transComp.worldTransform, textureAsset->GetTexture());
			}
		}
#endif

#if true
		for (auto&& [entity, transComp, textComp] : enttRegistry.view<TransformComponent, TextComponent>().each()) {
			auto fontAsset = AssetManager::GetAsset<FontAsset>(textComp.font);
			if (fontAsset) {
				Renderer2D::DrawString((uint32_t)entity, transComp.worldTransform, textComp.text, fontAsset->GetFont(), fontAsset->GetFontAtlas(), textComp.color);
			}
		}
#endif
		Renderer2D::End();

		_scene.GetParticleSystem().Render(_vpCB, _globalCB);
	}
}