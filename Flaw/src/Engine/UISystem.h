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

		Entity renderCamera;

		vec2 size;

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
		void UpdateUIObjectsRecurcive(Canvas& canvas, const vec2& parentScaledSize, Entity entity);
		void HandleImageComponentIfExists(Canvas& canvas, Entity entity, const mat4& worldTransform);

		void RenderImpl(CameraConstants& cameraConstants, DepthTest depthTest, bool depthWrite, RenderQueue& queue);

	private:
		Scene& _scene;

		Ref<GraphicsShader> _defaultUIImageShader;

		Ref<ConstantBuffer> _cameraConstantCB;

		std::unordered_map<Ref<Texture2D>, Ref<Material>> _uiImageMaterialMap;

		std::vector<Canvas> _canvases;
	};
}