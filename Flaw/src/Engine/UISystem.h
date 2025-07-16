#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "ECS/ECS.h"
#include "Entity.h"
#include "Components.h"
#include "Graphics.h"
#include "RenderQueue.h"
#include "Material.h"
#include "Camera.h"
#include "Fonts.h"

namespace flaw {
	class Scene;

	struct ImageRenderDataEntry {
		Ref<Material> material;
		std::vector<BatchedData> batchedDatas;
	};

	struct TextRenderDataEntry {
		Ref<Material> material;
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;
		std::vector<BatchedData> batchedDatas;
	};

	struct Canvas {
		CanvasComponent::RenderMode renderMode = CanvasComponent::RenderMode::ScreenSpaceOverlay;

		Entity renderCamera;

		vec2 size;

		std::unordered_map<Ref<Texture2D>, ImageRenderDataEntry> _imageRenderDataMap;
		std::unordered_map<Ref<Font>, TextRenderDataEntry> _textRenderDataMap;

		RenderQueue _renderQueue;
	};

	class UISystem {
	public:
		UISystem(Scene& scene);
		~UISystem();

		void Update();
		void Render();
		void Render(Ref<Camera> camera);

	private:
		void UpdateCanvasScalers();
		void UpdateCanvases();

		void UpdateUIObjectsInCanvasRecurcive(Canvas& canvas, const vec2& parentScaledSize, Entity entity);
		void HandleImageComponentIfExists(Canvas& canvas, Entity entity, const mat4& worldTransform);
		void HandleTextComponentIfExists(Canvas& canvas, Entity entity, const mat4& worldTransform);

		void RenderImpl(CameraConstants& cameraConstants, DepthTest depthTest, bool depthWrite, RenderQueue& queue);

	private:
		Scene& _scene;

		Ref<GraphicsShader> _defaultImageUIShader;
		Ref<GraphicsShader> _defaultTextUIShader;

		Ref<ConstantBuffer> _cameraConstantCB;

		std::vector<Canvas> _canvases;
	};
}