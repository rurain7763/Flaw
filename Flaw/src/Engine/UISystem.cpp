#include "pch.h"
#include "UISystem.h"
#include "Scene.h"
#include "RenderSystem.h"
#include "AssetManager.h"
#include "Assets.h"

namespace flaw {
	UISystem::UISystem(Scene& scene)
		: _scene(scene)
	{
		_defaultUIImageShader = Graphics::CreateGraphicsShader("Resources/Shaders/stdui_image.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_defaultUIImageShader->AddInputElement<float>("POSITION", 3);
		_defaultUIImageShader->AddInputElement<float>("TEXCOORD", 2);
		_defaultUIImageShader->CreateInputLayout();

		_cameraConstantCB = Graphics::CreateConstantBuffer(sizeof(CameraConstants));
	}

	UISystem::~UISystem() {
	}

	void UISystem::Update() {
		auto& registry = _scene.GetRegistry();

		_uiImageMaterialMap.clear();
		_canvases.clear();

;		for (auto&& [entity, transComp, rectLayoutComp, canvasComp] : registry.view<TransformComponent, RectLayoutComponent, CanvasComponent>().each()) {
			Entity canvasEntt(entity, &_scene);

			rectLayoutComp.anchorMin = vec2(0.5);
			rectLayoutComp.anchorMax = vec2(0.5);
			rectLayoutComp.pivot = vec2(0.5);

			_canvases.resize(_canvases.size() + 1);

			auto& canvas = _canvases.back();  
			canvas.renderMode = canvasComp.renderMode;
			canvas._overlayRenderQueue.Open();
			canvas._cameraRenderQueue.Open();
			canvas._worldSpaceRenderQueue.Open();

			canvasEntt.EachChildren([this, &transComp, &rectLayoutComp, &canvas](const Entity& childEntity) {
				UpdateUIObjectRecurcive(canvas, rectLayoutComp.sizeDelta * vec2(transComp.scale), childEntity);
			});

			canvas._overlayRenderQueue.Close();
			canvas._cameraRenderQueue.Close();
			canvas._worldSpaceRenderQueue.Close();
		}
	}

	void UISystem::UpdateUIObjectRecurcive(Canvas& canvas, const vec2& parentScaledSize, Entity entity) {
		auto& transComp = entity.GetComponent<TransformComponent>();
		auto& rectLayoutComp = entity.GetComponent<RectLayoutComponent>();

		vec3 worldPos, worldRotation, worladScale;
		ExtractModelMatrix(transComp.worldTransform, worldPos, worldRotation, worladScale);

		vec2 position = worldPos;
		vec2 size = worladScale;

		if (rectLayoutComp.IsLRStretched()) {
			size.x = (parentScaledSize.x * (rectLayoutComp.anchorMax.x - rectLayoutComp.anchorMin.x) + rectLayoutComp.sizeDelta.x) * worladScale.x;
		}
		else {
			size.x = rectLayoutComp.sizeDelta.x * worladScale.x;
			position.x = worldPos.x - (rectLayoutComp.pivot.x - 0.5) * size.x;
		}

		if (rectLayoutComp.IsTBStretched()) {
			size.y = (parentScaledSize.y * (rectLayoutComp.anchorMax.y - rectLayoutComp.anchorMin.y) + rectLayoutComp.sizeDelta.y) * worladScale.y;
		}
		else {
			size.y = rectLayoutComp.sizeDelta.y * worladScale.y;
			position.y = worldPos.y - (rectLayoutComp.pivot.y - 0.5) * size.y;
		}

		mat4 worldTransform = ModelMatrix(vec3(position, worldPos.z), worldRotation, vec3(size, 1.0));

		// Handle ImageComponent
		if (entity.HasComponent<ImageComponent>()) {
			auto& imageComp = entity.GetComponent<ImageComponent>();

			auto texAsset = AssetManager::GetAsset<Texture2DAsset>(imageComp.texture);
			if (texAsset) {
				Ref<Material> material;
				auto materialIt = _uiImageMaterialMap.find(texAsset->GetTexture());
				if (materialIt == _uiImageMaterialMap.end()) {
					material = CreateRef<Material>();
					material->shader = _defaultUIImageShader;
					material->renderMode = RenderMode::Transparent;
					material->cullMode = CullMode::None;
					material->depthTest = DepthTest::Disabled;
					material->depthWrite = false;
					material->albedoTexture = texAsset->GetTexture();
					_uiImageMaterialMap[texAsset->GetTexture()] = material;
				}
				else {
					material = materialIt->second;
				}

				auto quadMeshAsset = AssetManager::GetAsset<StaticMeshAsset>(AssetManager::DefaultStaticQuadMeshKey);
				if (canvas.renderMode == CanvasComponent::RenderMode::ScreenSpaceOverlay) {
					canvas._overlayRenderQueue.Push(quadMeshAsset->GetMesh(), worldTransform, material);
				}
				else if (canvas.renderMode == CanvasComponent::RenderMode::ScreenSpaceCamera) {
					canvas._cameraRenderQueue.Push(quadMeshAsset->GetMesh(), worldTransform, material);
				}
				else if (canvas.renderMode == CanvasComponent::RenderMode::WorldSpace) {
					canvas._worldSpaceRenderQueue.Push(quadMeshAsset->GetMesh(), worldTransform, material);
				}
			}
		}

		entity.EachChildren([this, &canvas, &size](const Entity& childEntity) {
			UpdateUIObjectRecurcive(canvas, size, childEntity);
		});
	}

	void UISystem::RenderImpl(CameraConstants& cameraConstants, RenderQueue& queue) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();
		auto materialCB = Graphics::GetMaterialConstantsCB();
		auto batchedTransformSB = Graphics::GetBatchedTransformSB();

		_cameraConstantCB->Update(&cameraConstants, sizeof(CameraConstants));

		while (!queue.Empty()) {
			auto& entry = queue.Front();

			pipeline->SetShader(entry.material->shader);
			pipeline->SetFillMode(FillMode::Solid);
			pipeline->SetCullMode(entry.material->cullMode);
			pipeline->SetDepthTest(entry.material->depthTest, entry.material->depthWrite);

			MaterialConstants materialConstants;
			entry.material->FillMaterialConstants(materialConstants);
			materialCB->Update(&materialConstants, sizeof(MaterialConstants));

			cmdQueue.SetPipeline(pipeline);
			cmdQueue.SetConstantBuffer(_cameraConstantCB, 0);
			cmdQueue.SetConstantBuffer(Graphics::GetGlobalConstantsCB(), 1);
			cmdQueue.SetConstantBuffer(materialCB, 2);
			cmdQueue.SetStructuredBuffer(batchedTransformSB, 0);

			if (entry.material->albedoTexture) {
				cmdQueue.SetTexture(entry.material->albedoTexture, ReservedTextureStartSlot);
			}
			if (entry.material->normalTexture) {
				cmdQueue.SetTexture(entry.material->normalTexture, ReservedTextureStartSlot + 1);
			}
			if (entry.material->emissiveTexture) {
				cmdQueue.SetTexture(entry.material->emissiveTexture, ReservedTextureStartSlot + 2);
			}
			if (entry.material->heightTexture) {
				cmdQueue.SetTexture(entry.material->heightTexture, ReservedTextureStartSlot + 3);
			}
			if (entry.material->metallicTexture) {
				cmdQueue.SetTexture(entry.material->metallicTexture, ReservedTextureStartSlot + 4);
			}
			if (entry.material->roughnessTexture) {
				cmdQueue.SetTexture(entry.material->roughnessTexture, ReservedTextureStartSlot + 5);
			}
			if (entry.material->ambientOcclusionTexture) {
				cmdQueue.SetTexture(entry.material->ambientOcclusionTexture, ReservedTextureStartSlot + 6);
			}
			for (int32_t i = 0; i < entry.material->cubeTextures.size(); ++i) {
				if (entry.material->cubeTextures[i]) {
					cmdQueue.SetTexture(entry.material->cubeTextures[i], CubeTextureStartSlot + i);
				}
			}
			for (int32_t i = 0; i < entry.material->textureArrays.size(); ++i) {
				if (entry.material->textureArrays[i]) {
					cmdQueue.SetTexture(entry.material->textureArrays[i], TextureArrayStartSlot + i);
				}
			}

			cmdQueue.Execute();

			for (auto& obj : entry.instancingObjects) {
				auto& mesh = obj.mesh;
				auto& meshSegment = mesh->GetMeshSegementAt(obj.segmentIndex);

				batchedTransformSB->Update(obj.modelMatrices.data(), obj.modelMatrices.size() * sizeof(mat4));

				cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
				cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
				cmdQueue.DrawIndexedInstanced(mesh->GetGPUIndexBuffer(), meshSegment.indexCount, obj.instanceCount, meshSegment.indexStart, meshSegment.vertexStart);

				cmdQueue.Execute();
			}

			queue.Pop();
		}
	}

	void UISystem::Render() {
		// TODO: Implement default camera rendering logic if needed
	}

	void UISystem::Render(Ref<Camera> camera) {
		CameraConstants defaultCameraConstants = {};
		defaultCameraConstants.position = camera->GetPosition();
		defaultCameraConstants.view = camera->GetViewMatrix();
		defaultCameraConstants.projection = camera->GetProjectionMatrix();

		for (auto& canvas : _canvases) {
			RenderImpl(defaultCameraConstants, canvas._worldSpaceRenderQueue);
		}
	}
}