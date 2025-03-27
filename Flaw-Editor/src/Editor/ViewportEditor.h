#pragma once

#include <Flaw.h>

#include "EditorCamera.h"

namespace flaw {
	class ViewportEditor {
	public:
		ViewportEditor(Application& app, EditorCamera& camera);
		~ViewportEditor();

		void OnRender();
		void SetScene(const Ref<Scene>& scene);

	private:
		void CreateRequiredTextures();

		uint32_t MousePicking(int32_t x, int32_t y);
		uint32_t MousePickingWithRay(const vec2& mousePos, const vec2& relativePos, const vec2& currentSize);

	private:
		PlatformContext& _platformContext;
		GraphicsContext& _graphicsContext;
		EventDispatcher& _eventDispatcher;
		EditorCamera& _camera;

		Ref<Scene> _scene;
		Entity _selectedEntt;

		Ref<Texture> _captureRenderTargetTexture;
		Ref<Texture> _idRenderTexture;
	};
}
