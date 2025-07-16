#pragma once

#include <Flaw.h>
#include "EditorTypes.h"

#include "EditorCamera.h"

namespace flaw {
	class ViewportEditor {
	public:
		ViewportEditor(Application& app, EditorCamera& camera);
		~ViewportEditor();

		void OnRender();
		void SetScene(const Ref<Scene>& scene);

		vec4 GetViewport() const { return _viewport; }

	private:
		void CreateRequiredTextures();

		uint32_t MousePicking(int32_t x, int32_t y);

		void DrawDebugComponent();

	private:
		PlatformContext& _platformContext;
		GraphicsContext& _graphicsContext;
		EventDispatcher& _eventDispatcher;
		EditorCamera& _editorCamera;

		bool _useEditorCamera;
		bool _selectionEnabled;

		Ref<Scene> _scene;
		Entity _selectedEntt;

		Ref<Texture2D> _captureRenderTargetTexture;

		vec4 _viewport = vec4(0.0f);
	};
}
