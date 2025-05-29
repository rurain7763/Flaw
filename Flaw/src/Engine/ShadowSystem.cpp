#include "pch.h"
#include "ShadowSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "AnimationSystem.h"

namespace flaw {
	ShadowSystem::ShadowSystem(Scene& scene)
		: _scene(scene)
	{
		Ref<GraphicsShader> shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap_skeletal.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
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
		_shadowMapSkeletalMaterial->cullMode = CullMode::Back;
		_shadowMapSkeletalMaterial->depthTest = DepthTest::Less;
		_shadowMapSkeletalMaterial->depthWrite = true;

		shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
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
		_shadowMapStaticMaterial->cullMode = CullMode::Back;
		_shadowMapStaticMaterial->depthTest = DepthTest::Less;
		_shadowMapStaticMaterial->depthWrite = true;

		_shadowUniformsCB = Graphics::CreateConstantBuffer(sizeof(ShadowUniforms));
	}

	Ref<GraphicsRenderPass> ShadowSystem::CreateShadowMapRenderPass() {
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

	void ShadowSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		if (registry.any_of<DirectionalLightComponent>(entity)) {
			auto& shadowMap = _directionalShadowMaps[(uint32_t)entity];
			shadowMap.renderPass = CreateShadowMapRenderPass();
		}

		if (registry.any_of<SpotLightComponent>(entity)) {
			auto& shadowMap = _spotLightShadowMaps[(uint32_t)entity];
			shadowMap.renderPass = CreateShadowMapRenderPass();
		}
	}

	void ShadowSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		_directionalShadowMaps.erase((uint32_t)entity);
		_spotLightShadowMaps.erase((uint32_t)entity);
	}

	void ShadowSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, transComp, lightComp] : registry.view<TransformComponent, DirectionalLightComponent>().each()) {
			auto& shadowMap = _directionalShadowMaps[(uint32_t)entity];

			// TODO: 현재는 하드코딩된 값으로 테스트
			const float orthoHalfSize = (ShadowMapSize / 100.f) / 2.0f;
			const vec3 origin = vec3(0.0f, 0.0f, 0.0f);

			shadowMap.uniforms.view = LookAt(origin, transComp.GetWorldPosition() + transComp.GetWorldFront(), Up);
			shadowMap.uniforms.projection = Orthographic(-orthoHalfSize, orthoHalfSize, -orthoHalfSize, orthoHalfSize, 0.1f, 10000.0f);

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		for (auto&& [entity, transform, lightComp] : registry.view<TransformComponent, SpotLightComponent>().each()) {
			auto& shadowMap = _spotLightShadowMaps[(uint32_t)entity];

			shadowMap.uniforms.view = LookAt(transform.GetWorldPosition(), transform.GetWorldPosition() + transform.GetWorldFront(), Up);
			shadowMap.uniforms.projection = Perspective(lightComp.outer * 2.0, 1.0f, 0.1f, 10000.0f);

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

	void ShadowSystem::Render(Ref<StructuredBuffer>& batchedTransformSB) {
		while (!_shadowMapRenderQueue.Empty()) {
			auto& entry = _shadowMapRenderQueue.Front();
			_shadowMapRenderQueue.Pop();

			for (const auto& [entt, shadowMap] : _directionalShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, shadowMap.uniforms, batchedTransformSB);
				shadowMap.renderPass->Unbind();
			}

			for (const auto& [entt, shadowMap] : _spotLightShadowMaps) {
				shadowMap.renderPass->Bind(false, false);
				DrawRenderEntry(entry, shadowMap.uniforms, batchedTransformSB);
				shadowMap.renderPass->Unbind();
			}
		}
	}

	void ShadowSystem::DrawRenderEntry(const RenderEntry& entry, const ShadowUniforms& shadowUniforms, Ref<StructuredBuffer>& batchedTransformSB) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();
		
		pipeline->SetShader(entry.material->shader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(entry.material->cullMode);
		pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

		cmdQueue.SetPipeline(pipeline);

		_shadowUniformsCB->Update(&shadowUniforms, sizeof(ShadowUniforms));
		cmdQueue.SetConstantBuffer(_shadowUniformsCB, 0);

		cmdQueue.SetStructuredBuffer(batchedTransformSB, 0);

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
			cmdQueue.SetStructuredBuffer(obj.skeletonBoneMatrices, 1);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
			cmdQueue.Execute();
		}
	}
}