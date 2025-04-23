#pragma once

#include <Flaw.h>

#include "EditorTypes.h"
#include "EditorCamera.h"
#include "Editor/OutlinerEditor.h"
#include "Editor/ViewportEditor.h"
#include "Editor/ContentBrowserEditor.h"
#include "Editor/DetailsEditor.h"
#include "Editor/LandscapeEditor.h"
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

		void UpdateSceneAsEditorMode(const Ref<Scene>& scene);

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

		OutlinerEditor _outlinerEditor;
		ViewportEditor _viewportEditor;
		ContentBrowserEditor _contentBrowserEditor;
		DetailsEditor _detailsEditor;
		LogEditor _logEditor;
		LandscapeEditor _landscapeEditor;

		Ref<Scene> _editorScene;
		Ref<Scene> _runtimeScene;
		SceneState _sceneState;
		bool _pause;

		Ref<Texture2D> _playButtonTex;
		Ref<Texture2D> _stopButtonTex;
		Ref<Texture2D> _pauseButtonTex;
		Ref<Texture2D> _resumeButtonTex;

		EditorCamera _camera;
		std::string _currentScenePath;

		EditorMode _editorMode;
	};
}

