#include "pch.h"
#include "UISystem.h"
#include "Scene.h"
#include "RenderSystem.h"
#include "AssetManager.h"
#include "Assets.h"
#include "TransformSystem.h"

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

		for (auto&& [entity, transComp, rectLayoutComp, canvasComp, canvasScaler] : registry.view<TransformComponent, RectLayoutComponent, CanvasComponent, CanvasScalerComponent>().each()) {
			Entity canvasEntt(entity, &_scene);

			if (canvasEntt.HasParent()) {
				continue;
			}

			rectLayoutComp.anchorMin = vec2(0.5);
			rectLayoutComp.anchorMax = vec2(0.5);
			rectLayoutComp.pivot = vec2(0.5);

			if (canvasComp.renderMode == CanvasComponent::RenderMode::WorldSpace) {
				_scene.GetTransformSystem().UpdateTransformImmediate(canvasEntt);
				continue;
			}

			int32_t width, height;
			Graphics::GetSize(width, height);

			rectLayoutComp.sizeDelta = vec2((float)width, (float)height);

			if (canvasComp.renderMode == CanvasComponent::RenderMode::ScreenSpaceOverlay) {
				transComp.position = vec3(0.0f);
				transComp.rotation = vec3(0.0f);
				transComp.scale = vec3(1.0f);
			}
			else if (canvasComp.renderMode == CanvasComponent::RenderMode::ScreenSpaceCamera) {
				Entity cameraEntt = _scene.FindEntityByUUID(canvasComp.renderCamera);
				if (cameraEntt) {
					// TODO: under from this line, codes must have performance issue
					auto& cameraTransComp = cameraEntt.GetComponent<TransformComponent>();
					auto& cameraComp = cameraEntt.GetComponent<CameraComponent>();

					if (cameraComp.perspective) {
						const float fovY = cameraComp.fov;
						const float height = (tan(fovY * 0.5f) * canvasComp.planeDistance) * 2.0f;
						const float width = height * cameraComp.aspectRatio;
						transComp.scale.x = width / rectLayoutComp.sizeDelta.x;
						transComp.scale.y = height / rectLayoutComp.sizeDelta.y;
					}
					else {
						const float orthoHeight = cameraComp.orthoSize;
						const float height = orthoHeight * 2.0f;
						const float width = orthoHeight * cameraComp.aspectRatio * 2.0f;
						transComp.scale.x = width / rectLayoutComp.sizeDelta.x;
						transComp.scale.y = height / rectLayoutComp.sizeDelta.y;
					}

					transComp.position = cameraTransComp.position + cameraTransComp.GetWorldFront() * canvasComp.planeDistance;
					transComp.rotation = cameraTransComp.rotation;
				}
			}

			_scene.GetTransformSystem().UpdateTransformImmediate(canvasEntt);
		}

;		for (auto&& [entity, transComp, rectLayoutComp, canvasComp] : registry.view<TransformComponent, RectLayoutComponent, CanvasComponent>().each()) {
			Entity canvasEntt(entity, &_scene);

			if (canvasEntt.HasParent()) {
				continue;
			}

			_canvases.resize(_canvases.size() + 1);

			auto& canvas = _canvases.back();  
			canvas.renderMode = canvasComp.renderMode;
			canvas.renderCamera = _scene.FindEntityByUUID(canvasComp.renderCamera);
			canvas.size = rectLayoutComp.sizeDelta * vec2(transComp.GetWorldScale());
			canvas._renderQueue.Open();

			canvasEntt.EachChildren([this, &transComp, &rectLayoutComp, &canvas](const Entity& childEntity) {
				UpdateUIObjectsRecurcive(canvas, rectLayoutComp.sizeDelta * vec2(transComp.scale), childEntity);
			});

			canvas._renderQueue.Close();
		}
	}

	void UISystem::UpdateUIObjectsRecurcive(Canvas& canvas, const vec2& parentScaledSize, Entity entity) {
		auto& transComp = entity.GetComponent<TransformComponent>();
		auto& rectLayoutComp = entity.GetComponent<RectLayoutComponent>();

		vec3 worldPos, worldRotation, worladScale;
		ExtractModelMatrix(transComp.worldTransform, worldPos, worldRotation, worladScale);

		vec2 position = worldPos;
		vec2 size = worladScale;

		if (rectLayoutComp.IsLRStretched()) {
			size.x = parentScaledSize.x * (rectLayoutComp.anchorMax.x - rectLayoutComp.anchorMin.x) + rectLayoutComp.sizeDelta.x;
		}
		else {
			size.x = rectLayoutComp.sizeDelta.x * worladScale.x;
			position.x = worldPos.x - (rectLayoutComp.pivot.x - 0.5) * size.x;
		}

		if (rectLayoutComp.IsTBStretched()) {
			size.y = parentScaledSize.y * (rectLayoutComp.anchorMax.y - rectLayoutComp.anchorMin.y) + rectLayoutComp.sizeDelta.y;
		}
		else {
			size.y = rectLayoutComp.sizeDelta.y * worladScale.y;
			position.y = worldPos.y - (rectLayoutComp.pivot.y - 0.5) * size.y;
		}

		mat4 worldTransform = ModelMatrix(vec3(position, worldPos.z), worldRotation, vec3(size, 1.0));

		HandleImageComponentIfExists(canvas, entity, worldTransform);

		entity.EachChildren([this, &canvas, &size](const Entity& childEntity) {
			UpdateUIObjectsRecurcive(canvas, size, childEntity);
		});
	}

	void UISystem::HandleImageComponentIfExists(Canvas& canvas, Entity entity, const mat4& worldTransform) {
		if (!entity.HasComponent<ImageComponent>()) {
			return;
		}

		auto& imageComp = entity.GetComponent<ImageComponent>();

		auto texAsset = AssetManager::GetAsset<Texture2DAsset>(imageComp.texture);
		if (!texAsset) {
			return;
		}

		Ref<Material> material;
		auto materialIt = _uiImageMaterialMap.find(texAsset->GetTexture());
		if (materialIt == _uiImageMaterialMap.end()) {
			material = CreateRef<Material>();
			material->shader = _defaultUIImageShader;
			material->renderMode = RenderMode::Transparent;
			material->cullMode = CullMode::None;
			material->albedoTexture = texAsset->GetTexture();
			_uiImageMaterialMap[texAsset->GetTexture()] = material;
		}
		else {
			material = materialIt->second;
		}

		auto quadMeshAsset = AssetManager::GetAsset<StaticMeshAsset>(AssetManager::DefaultStaticQuadMeshKey);

		canvas._renderQueue.Push(quadMeshAsset->GetMesh(), worldTransform, material);
	}

	void UISystem::RenderImpl(CameraConstants& cameraConstants, DepthTest depthTest, bool depthWrite, RenderQueue& queue) {
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
			pipeline->SetDepthTest(depthTest, depthWrite);

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
		CameraConstants cameraConstants = {};

		for (auto& canvas : _canvases) {
			if (canvas.renderMode == CanvasComponent::RenderMode::WorldSpace || canvas.renderMode == CanvasComponent::RenderMode::ScreenSpaceCamera) {
				if (!canvas.renderCamera) {
					continue;
				}

				auto& transComp = canvas.renderCamera.GetComponent<TransformComponent>();
				auto& cameraComp = canvas.renderCamera.GetComponent<CameraComponent>();

				vec3 cameraPosition = transComp.GetWorldPosition();
				vec3 cameraFront = transComp.GetWorldFront();

				cameraConstants.position = cameraPosition;
				cameraConstants.view = LookAt(cameraPosition, cameraPosition + cameraFront, Up);
				cameraConstants.projection = cameraComp.GetProjectionMatrix();

				RenderImpl(cameraConstants, DepthTest::Less, true, canvas._renderQueue);
			}
			else if (canvas.renderMode == CanvasComponent::RenderMode::ScreenSpaceOverlay) {
				vec2 halfSize = canvas.size * 0.5f;

				cameraConstants.position = vec3(0.0f);
				cameraConstants.view = mat4(1.0f);
				cameraConstants.projection = Orthographic(-halfSize.x, halfSize.x, -halfSize.y, halfSize.y, -1.0f, 1.0f);

				RenderImpl(cameraConstants, DepthTest::Disabled, false, canvas._renderQueue);
			}
		}
	}

	void UISystem::Render(Ref<Camera> camera) {
		CameraConstants cameraConstants = {};
		cameraConstants.position = camera->GetPosition();
		cameraConstants.view = camera->GetViewMatrix();
		cameraConstants.projection = camera->GetProjectionMatrix();

		for (auto& canvas : _canvases) {
			RenderImpl(cameraConstants, DepthTest::Less, true, canvas._renderQueue);
		}
	}
}