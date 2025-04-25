#pragma once

#include <Flaw.h>
#include "EditorCamera.h"
#include "ViewportEditor.h"
#include "ContentBrowserEditor.h"

namespace flaw {
	class LandscapeEditor {
	public:
		LandscapeEditor(
			Application& app, 
			EditorCamera& editorCam,
			ViewportEditor& viewportEditor,
			ContentBrowserEditor& contentEditor
		);

		~LandscapeEditor();

		void OnRender();

		void SetScene(const Ref<Scene>& scene);

	private:
		bool GetBrushPos(vec2& pos);
		void UpdateLandscapeTexture(const Ref<Texture2D>& texture);

	private:
		enum class BrushType {
			Round,
			Texture,
		};

		struct LandscapeUniform {
			float deltaTime;

			uint32_t brushType;
			vec2 brushPos;

			uint32_t width;
			uint32_t height;

			uint32_t padding[2];
		};

		Application& _app;
		EditorCamera& _editorCamera;
		ViewportEditor& _viewportEditor;
		ContentBrowserEditor& _contentEditor;

		Ref<Scene> _scene;

		Entity _selectedEntt;

		BrushType _brushType = BrushType::Round;
		vec2 _brushPos = vec2(0.0f);
		Ref<Texture2DAsset> _brushTextureAsset;

		Ref<ConstantBuffer> _landscapeUniformCB;

		Ref<ComputeShader> _landscapeShader;
		Ref<ComputePipeline> _landscapePipeline;
	};
}