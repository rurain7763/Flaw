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

namespace flaw {
	class Scene;

	struct Canvas {
		CanvasComponent::RenderMode renderMode = CanvasComponent::RenderMode::ScreenSpaceOverlay;

		RenderQueue _overlayRenderQueue;
		RenderQueue _cameraRenderQueue;
		RenderQueue _worldSpaceRenderQueue;
	};

	class UISystem {
	public:
		UISystem(Scene& scene);
		~UISystem();

		void Update();
		void Render();
		void Render(Ref<Camera> camera);

	private:
		void UpdateUIObjectRecurcive(Canvas& canvas, const vec2& parentScaledSize, Entity entity);

		void RenderImpl(CameraConstants& cameraConstants, RenderQueue& queue);

	private:
		Scene& _scene;

		Ref<GraphicsShader> _defaultUIImageShader;

		Ref<ConstantBuffer> _cameraConstantCB;

		std::unordered_map<Ref<Texture2D>, Ref<Material>> _uiImageMaterialMap;

		std::vector<Canvas> _canvases;
	};
}