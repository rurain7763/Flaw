#include "pch.h"
#include "ShadowSystem.h"
#include "Scene.h"
#include "Components.h"
#include "PrimitiveManager.h"
#include "AssetManager.h"
#include "Assets.h"

namespace flaw {
	ShadowSystem::ShadowSystem(Scene& scene)
		: _scene(scene)
	{
		Ref<GraphicsShader> shadowMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/shadowmap.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shadowMapShader->AddInputElement<float>("POSITION", 3);
		shadowMapShader->AddInputElement<float>("TEXCOORD", 2);
		shadowMapShader->AddInputElement<float>("TANGENT", 3);
		shadowMapShader->AddInputElement<float>("NORMAL", 3);
		shadowMapShader->AddInputElement<float>("BINORMAL", 3);
		shadowMapShader->CreateInputLayout();

		_shadowMapMaterial = CreateRef<Material>();
		_shadowMapMaterial->shader = shadowMapShader;
		_shadowMapMaterial->renderMode = RenderMode::Opaque;
		_shadowMapMaterial->cullMode = CullMode::Back;
		_shadowMapMaterial->depthTest = DepthTest::Less;
		_shadowMapMaterial->depthWrite = true;

		_shadowUniformsCB = Graphics::CreateConstantBuffer(sizeof(ShadowUniforms));
	}

	void ShadowSystem::RegisterEntity(entt::registry& registry, entt::entity entity) {
		//if (registry.any_of<DirectionalLightComponent>(entity) || registry.any_of<PointLightComponent>(entity) || registry.any_of<SpotLightComponent>(entity)) {
		if (registry.any_of<DirectionalLightComponent>(entity)) {
			auto& enttComp = registry.get<EntityComponent>(entity);

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
			shadowMapDesc.renderTargets[0].clearValue = { 1.0f, 1.0f, 1.0f, 0.0f };
			shadowMapDesc.renderTargets[0].resizeFunc = [](Ref<Texture2D>& current, int32_t width, int32_t height) -> Ref<Texture2D> { return current; };

			texDesc.bindFlags = BindFlag::DepthStencil;
			texDesc.format = PixelFormat::D24S8_UINT;
			shadowMapDesc.depthStencil.texture = Graphics::CreateTexture2D(texDesc);
			shadowMapDesc.depthStencil.resizeFunc = [](Ref<Texture2D>& current, int32_t width, int32_t height) -> Ref<Texture2D> { return current; };

			ShadowMap shadowMap;
			shadowMap.renderPass = Graphics::CreateRenderPass(shadowMapDesc);
			
			_shadowMaps[enttComp.uuid] = shadowMap;
		}
	}

	void ShadowSystem::UnregisterEntity(entt::registry& registry, entt::entity entity) {
		auto& enttComp = registry.get<EntityComponent>(entity);
		if (_shadowMaps.find(enttComp.uuid) != _shadowMaps.end()) {
			_shadowMaps.erase(enttComp.uuid);
		}
	}

	void ShadowSystem::Update() {
		auto& registry = _scene.GetRegistry();

		for (auto&& [entity, enttComp, transComp, lightComp] : registry.view<EntityComponent, TransformComponent, DirectionalLightComponent>().each()) {
			auto& shadowMap = _shadowMaps[enttComp.uuid];

			// TODO: 현재는 하드코딩된 값으로 테스트
			const float orthoHalfSize = (ShadowMapSize / 100.f) / 2.0f;

			shadowMap.uniforms.view = LookAt(transComp.GetWorldPosition(), transComp.GetWorldPosition() + transComp.GetWorldFront(), Up);
			shadowMap.uniforms.projection = Orthographic(-orthoHalfSize, orthoHalfSize, -orthoHalfSize, orthoHalfSize, 0.1f, 10000.0f);

			shadowMap.renderPass->ClearAllRenderTargets();
			shadowMap.renderPass->ClearDepthStencil();
		}

		_shadowMapRenderQueue.Open();

		for (auto&& [entity, transform, skeletaMeshComp] : registry.view<TransformComponent, SkeletalMeshComponent>().each()) {
			if (skeletaMeshComp.castShadow) {
				auto& skeletalMeshAsset = AssetManager::GetAsset<SkeletalMeshAsset>(skeletaMeshComp.mesh);
				if (skeletalMeshAsset == nullptr) {
					continue;
				}

				// TODO: may be need frustum culling
				_shadowMapRenderQueue.Push(skeletalMeshAsset->GetMesh(), transform.worldTransform, _shadowMapMaterial);
			}
		}

		_shadowMapRenderQueue.Close();
	}

	void ShadowSystem::Render(Ref<StructuredBuffer>& batchedTransformSB) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		ShadowUniforms shadowUniforms;

		while (!_shadowMapRenderQueue.Empty()) {
			auto& entry = _shadowMapRenderQueue.Front();
			_shadowMapRenderQueue.Pop();

			for (const auto& [uuid, shadowMap] : _shadowMaps) {
				shadowMap.renderPass->Bind(false, false);

				cmdQueue.Begin();

				pipeline->SetShader(entry.material->shader);
				pipeline->SetFillMode(FillMode::Solid);
				pipeline->SetCullMode(entry.material->cullMode);
				pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

				cmdQueue.SetPipeline(pipeline);

				_shadowUniformsCB->Update(&shadowMap.uniforms, sizeof(ShadowUniforms));
				cmdQueue.SetConstantBuffer(_shadowUniformsCB, 0);

				cmdQueue.SetStructuredBuffer(batchedTransformSB, 0);
				cmdQueue.End();
				cmdQueue.Execute();

				// instancing draw
				for (auto& obj : entry.instancingObjects) {
					auto& mesh = obj.mesh;
					auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

					batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

					cmdQueue.Begin();
					cmdQueue.SetPrimitiveTopology(meshSegment.topology);
					cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
					cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);
					cmdQueue.End();
					cmdQueue.Execute();
				}

				shadowMap.renderPass->Unbind();
			}
		}
	}
}