#include "pch.h"
#include "ShadowSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "AnimationSystem.h"
#include "RenderSystem.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	ShadowSystem::ShadowSystem(Scene& scene)
		: _scene(scene)
	{
		Ref<GraphicsShader> shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap_skeletal.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		shadowMapShader->AddInputElement<float>("POSITION", 3);
		shadowMapShader->AddInputElement<float>("TEXCOORD", 2);
		shadowMapShader->AddInputElement<float>("TANGENT", 3);
		shadowMapShader->AddInputElement<float>("NORMAL", 3);
		shadowMapShader->AddInputElement<float>("BINORMAL", 3);
		shadowMapShader->AddInputElement<int32_t>("BONEINDICES", 4);
		shadowMapShader->AddInputElement<float>("BONEWEIGHTS", 4);
		shadowMapShader->CreateInputLayout();

		_shadowMapSkeletalMaterial = CreateRef<Material>();
		_shadowMapSkeletalMaterial->shader = shadowMapShader;
		_shadowMapSkeletalMaterial->renderMode = RenderMode::Opaque;
		_shadowMapSkeletalMaterial->cullMode = CullMode::Front;
		_shadowMapSkeletalMaterial->depthTest = DepthTest::Less;
		_shadowMapSkeletalMaterial->depthWrite = true;

		shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		shadowMapShader->AddInputElement<float>("POSITION", 3);
		shadowMapShader->AddInputElement<float>("TEXCOORD", 2);
		shadowMapShader->AddInputElement<float>("TANGENT", 3);
		shadowMapShader->AddInputElement<float>("NORMAL", 3);
		shadowMapShader->AddInputElement<float>("BINORMAL", 3);
		shadowMapShader->AddInputElement<int32_t>("BONEINDICES", 4);
		shadowMapShader->AddInputElement<float>("BONEWEIGHTS", 4);
		shadowMapShader->CreateInputLayout();

		_shadowMapStaticMaterial = CreateRef<Material>();
		_shadowMapStaticMaterial->shader = shadowMapShader;
		_shadowMapStaticMaterial->renderMode = RenderMode::Opaque;
		_shadowMapStaticMaterial->cullMode = CullMode::Front;
		_shadowMapStaticMaterial->depthTest = DepthTest::Less;
		_shadowMapStaticMaterial->depthWrite = true;

		_shadowUniformsCB = Graphics::CreateConstantBuffer(sizeof(ShadowUniforms));

		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(LightVPMatrix);
		sbDesc.count = MaxLightVPCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;

		_lightVPMatricesSB = Graphics::CreateStructuredBuffer(sbDesc);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreateDirectionalLightShadowMapRenderPass() {
		Texture2D::Descriptor texDesc = {};
		texDesc.width = ShadowMapSize;
		texDesc.height = ShadowMapSize;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTexture2D(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = ShadowMapSize;
		shadowMapDesc.renderTargets[0].viewportHeight = ShadowMapSize;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTexture2D(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreateSpotLightShadowMapRenderPass() {
		Texture2D::Descriptor texDesc = {};
		texDesc.width = ShadowMapSize;
		texDesc.height = ShadowMapSize;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTexture2D(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = ShadowMapSize;
		shadowMapDesc.renderTargets[0].viewportHeight = ShadowMapSize;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTexture2D(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreatePointLightShadowMapRenderPass() {
		TextureCube::Descriptor texDesc = {};
		texDesc.width = ShadowMapSize;
		texDesc.height = ShadowMapSize;
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::RenderTarget | BindFlag::ShaderResource;
		texDesc.format = PixelFormat::R32F;

		GraphicsRenderPass::Descriptor shadowMapDesc = {};
		shadowMapDesc.renderTargets.resize(1);
		shadowMapDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		shadowMapDesc.renderTargets[0].texture = Graphics::CreateTextureCube(texDesc);
		shadowMapDesc.renderTargets[0].viewportX = 0;
		shadowMapDesc.renderTargets[0].viewportY = 0;
		shadowMapDesc.renderTargets[0].viewportWidth = ShadowMapSize;
		shadowMapDesc.renderTargets[0].viewportHeight = ShadowMapSize;
		shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };

		texDesc.bindFlags = BindFlag::DepthStencil;
		texDesc.format = PixelFormat::D24S8_UINT;
		shadowMapDesc.depthStencil.texture = Graphics::CreateTextureCube(texDesc);

		return Graphics::CreateRenderPass(shadowMapDesc);
	}

	void ShadowSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		if (registry.any_of<DirectionalLightComponent>(entity)) {
			auto& shadowMap = _directionalShadowMaps[(uint32_t)entity];
			shadowMap.renderPass = CreateDirectionalLightShadowMapRenderPass();
		}

		if (registry.any_of<SpotLightComponent>(entity)) {
			auto& shadowMap = _spotLightShadowMaps[(uint32_t)entity];
			shadowMap.renderPass = CreateSpotLightShadowMapRenderPass();
		}

		if (registry.any_of<PointLightComponent>(entity)) {
			auto& shadowMap = _pointLightShadowMaps[(uint32_t)entity];
			shadowMap.renderPass = CreatePointLightShadowMapRenderPass();
		}
	}

	void ShadowSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_directionalShadowMaps.erase((uint32_t)entity);
		_spotLightShadowMaps.erase((uint32_t)entity);
		_pointLightShadowMaps.erase((uint32_t)entity);
	}

	void ShadowSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transComp, lightComp] : registry.view<TransformComponent, DirectionalLightComponent>().each()) {
			auto& shadowMap = _directionalShadowMaps[(uint32_t)entity];

			// not calculating here, because we need to calculate tight bounding box for directional light
			shadowMap.lightDirection = transComp.GetWorldFront();

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		for (auto&& [entity, transform, lightComp] : registry.view<TransformComponent, SpotLightComponent>().each()) {
			auto& shadowMap = _spotLightShadowMaps[(uint32_t)entity];

			shadowMap._lightVPMatrix.view = LookAt(transform.GetWorldPosition(), transform.GetWorldPosition() + transform.GetWorldFront(), Up);
			shadowMap._lightVPMatrix.projection = Perspective(lightComp.outer * 2.0, 1.0f, 0.1f, lightComp.range);

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		for (auto&& [entity, transComp, lightComp] : registry.view<TransformComponent, PointLightComponent>().each()) {
			auto& shadowMap = _pointLightShadowMaps[(uint32_t)entity];

			const vec3 faceDirections[6] = {
				Right, -Right, Up, -Up, Forward, -Forward
			};

			const vec3 upDirections[6] = {
				Up, Up, -Forward, Forward, Up, Up
			};

			for (int i = 0; i < 6; ++i) {
				shadowMap._lightVPMatrices[i].view = LookAt(transComp.GetWorldPosition(), transComp.GetWorldPosition() + faceDirections[i], upDirections[i]);
				shadowMap._lightVPMatrices[i].projection = Perspective(glm::half_pi<float>(), 1.0f, 0.1f, lightComp.range);
			}

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		_shadowMapRenderQueue.Open();

		for (auto&& [entity, transform, staticMeshComp] : registry.view<TransformComponent, StaticMeshComponent>().each()) {
			if (staticMeshComp.castShadow) {
				auto& staticMeshAsset = AssetManager::GetAsset<StaticMeshAsset>(staticMeshComp.mesh);
				if (staticMeshAsset == nullptr) {
					continue;
				}

				_shadowMapRenderQueue.Push(staticMeshAsset->GetMesh(), transform.worldTransform, _shadowMapStaticMaterial);
			}
		}

		for (auto&& [entity, transform, skeletalMeshComp] : _scene.GetRegistry().view<TransformComponent, SkeletalMeshComponent>().each()) {
			if (!skeletalMeshComp.castShadow) {
				continue;
			}

			auto meshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletalMeshComp.mesh);
			if (meshAsset == nullptr) {
				continue;
			}

			Ref<Mesh> mesh = meshAsset->GetMesh();

			auto& skeletalAnimData = _scene.GetAnimationSystem().GetSkeletalAnimationData((uint32_t)entity);
			auto boneMatrices = skeletalAnimData.GetBoneMatrices();
			if (boneMatrices == nullptr) {
				continue;
			}

			_shadowMapRenderQueue.Push(mesh, transform.worldTransform, _shadowMapSkeletalMaterial, boneMatrices);
		}

		_shadowMapRenderQueue.Close();
	}

	void ShadowSystem::CalcTightDirectionalLightMatrices(const Frustum& frustum, const vec3& lightDirection, mat4& outView, mat4& outProjection) {
		std::array<vec3, 8> worldSpaceCorners = frustum.GetCorners();

		mat4 lightViewMatrix = LookAt(vec3(0.0), lightDirection, Up);

		// # calculate corners coordinates in light space and get min, max coord elements
		vec3 minCornerInLightView = vec3(std::numeric_limits<float>::max());
		vec3 maxCornerInLightView = vec3(std::numeric_limits<float>::lowest());
		for (int32_t i = 0; i < worldSpaceCorners.size(); ++i) {
			const vec3& worldSpaceCorner = worldSpaceCorners[i];
			const vec3 lightSpaceCorner = lightViewMatrix * vec4(worldSpaceCorner, 1.0f);

			minCornerInLightView = glm::min(minCornerInLightView, lightSpaceCorner);
			maxCornerInLightView = glm::max(maxCornerInLightView, lightSpaceCorner);
		}

		// # get center position of aabb for light position
		vec3 lightPosition = (minCornerInLightView + maxCornerInLightView) * 0.5f;
		lightPosition.z = minCornerInLightView.z;

		// # translate light position to world space
		mat4 invLightViewMatrix = glm::inverse(lightViewMatrix);
		lightPosition = invLightViewMatrix * vec4(lightPosition, 1.0f);

		// # calculate view matrix
		outView = LookAt(lightPosition, lightPosition + lightDirection, Up);

		// # get aabb coordinates in new light view space
		minCornerInLightView = vec3(std::numeric_limits<float>::max());
		maxCornerInLightView = vec3(std::numeric_limits<float>::lowest());
		for (int32_t i = 0; i < worldSpaceCorners.size(); ++i) {
			const vec3& worldSpaceCorner = worldSpaceCorners[i];
			vec3 lightSpaceCorner = outView * vec4(worldSpaceCorner, 1.0f);

			minCornerInLightView = glm::min(minCornerInLightView, lightSpaceCorner);
			maxCornerInLightView = glm::max(maxCornerInLightView, lightSpaceCorner);
		}

		// # calculate projection matrix
		outProjection = Orthographic(minCornerInLightView.x, maxCornerInLightView.x, minCornerInLightView.y, maxCornerInLightView.y, minCornerInLightView.z, maxCornerInLightView.z);
	}

	void ShadowSystem::Render(CameraRenderStage& stage, Ref<StructuredBuffer>& batchedTransformSB) {
		while (!_shadowMapRenderQueue.Empty()) {
			auto& entry = _shadowMapRenderQueue.Front();
			_shadowMapRenderQueue.Pop();

			for (auto& [entt, shadowMap] : _directionalShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				CalcTightDirectionalLightMatrices(stage.frustum, shadowMap.lightDirection, shadowMap._lightVPMatrix.view, shadowMap._lightVPMatrix.projection);
				DrawRenderEntry(entry, batchedTransformSB, &shadowMap._lightVPMatrix, 1);
				shadowMap.renderPass->Unbind();
			}

			for (const auto& [entt, shadowMap] : _spotLightShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, batchedTransformSB, &shadowMap._lightVPMatrix, 1);
				shadowMap.renderPass->Unbind();
			}

			for (const auto& [entt, shadowMap] : _pointLightShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, batchedTransformSB, shadowMap._lightVPMatrices.data(), 6);
				shadowMap.renderPass->Unbind();
			}
		}
	}

	void ShadowSystem::DrawRenderEntry(const RenderEntry& entry, Ref<StructuredBuffer>& batchedTransformSB, const LightVPMatrix* lightVPMatrices, int32_t lightVPMatrixCount) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();
		
		pipeline->SetShader(entry.material->shader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(entry.material->cullMode);
		pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

		ShadowUniforms shadowUniforms = {};
		shadowUniforms.lightVPMatrixCount = lightVPMatrixCount;

		_shadowUniformsCB->Update(&shadowUniforms, sizeof(ShadowUniforms));
		_lightVPMatricesSB->Update(lightVPMatrices, lightVPMatrixCount * sizeof(LightVPMatrix));

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(_shadowUniformsCB, 0);
		cmdQueue.SetStructuredBuffer(batchedTransformSB, 0);
		cmdQueue.SetStructuredBuffer(_lightVPMatricesSB, 1);

		cmdQueue.Execute();

		// instancing draw
		for (auto& obj : entry.instancingObjects) {
			auto& mesh = obj.mesh;
			auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

			batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

			cmdQueue.SetPrimitiveTopology(meshSegment.topology);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
			cmdQueue.Execute();
		}

		for (auto& obj : entry.skeletalInstancingObjects) {
			auto& mesh = obj.mesh;
			auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

			batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

			cmdQueue.SetPrimitiveTopology(meshSegment.topology);
			cmdQueue.SetStructuredBuffer(obj.skeletonBoneMatrices, 2);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
			cmdQueue.Execute();
		}
	}
}