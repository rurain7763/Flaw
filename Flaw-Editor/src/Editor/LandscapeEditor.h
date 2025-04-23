#pragma once

#include <Flaw.h>
#include "EditorCamera.h"
#include "ViewportEditor.h"

namespace flaw {
	class LandscapeEditor {
	public:
		LandscapeEditor(
			Application& app, 
			EditorCamera& editorCam,
			ViewportEditor& viewportEditor
		);

		void OnRender();

		void SetScene(const Ref<Scene>& scene) { _scene = scene; }

	private:
		void UpdateLandscapeTexture();

	private:
		Application& _app;
		EditorCamera& _editorCamera;
		ViewportEditor& _viewportEditor;

		Ref<Scene> _scene;

		struct LandscapeUniform {
			uint32_t width;
			uint32_t height;
		};

		Ref<ConstantBuffer> _landscapeUniformCB;
		Ref<Texture2D> _landscapeTexture;

		Ref<ComputeShader> _landscapeShader;
		Ref<ComputePipeline> _landscapePipeline;
	};
}