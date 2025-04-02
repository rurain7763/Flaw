#pragma once

#include <Flaw.h>

#include "EditorTypes.h"
#include "EditorCamera.h"
#include "Editor/OutlinerEditor.h"
#include "Editor/ViewportEditor.h"
#include "Editor/ContentBrowserEditor.h"
#include "Editor/DetailsEditor.h"
#include "Editor/LogEditor.h"

namespace flaw {
	class EditorLayer : public Layer {
	public:
		EditorLayer(Application& app);

		void OnAttatch() override;
		void OnDetach() override;
		void OnUpdate() override;

		void SetTheme(EditorTheme theme);

	private:
		void CreateRequiredTextures();

		void RenderTargetScene(const Ref<Scene>& scene);

		void OnRenderToolbar();

		void OnScenePlay();
		void OnScenePause();
		void OnSceneResume();
		void OnSceneStop();

		void NewScene();
		void SaveScene();
		void SaveSceneAs();
		void OpenScene();
		void OpenScene(const char* path);

		void OpenProject();

	private:
		Application& _app;
		GraphicsContext& _graphicsContext;

		Ref<Scene> _editorScene;
		Ref<Scene> _runtimeScene;

		OutlinerEditor _outlinerEditor;
		ViewportEditor _viewportEditor;
		ContentBrowserEditor _contentBrowserEditor;
		DetailsEditor _detailsEditor;
		LogEditor _logEditor;

		SceneState _sceneState;

		bool _pause;

		Ref<Texture> _playButtonTex;
		Ref<Texture> _stopButtonTex;
		Ref<Texture> _pauseButtonTex;
		Ref<Texture> _resumeButtonTex;

		EditorCamera _camera;
		std::string _currentScenePath;
	};
}

