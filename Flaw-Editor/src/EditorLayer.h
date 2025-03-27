#pragma once

#include <Flaw.h>

#include "EditorCamera.h"
#include "Editor/OutlinerEditor.h"
#include "Editor/ViewportEditor.h"
#include "Editor/ContentBrowserEditor.h"
#include "Editor/LogEditor.h"

namespace flaw {
	class EditorLayer : public Layer {
	public:
		enum class Theme {
			Orange,
			Dark,
			Light,
			GrayOrange
		};

		EditorLayer(Application& app);

		void OnAttatch() override;
		void OnDetach() override;
		void OnUpdate() override;

		void SetTheme(Theme theme);

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
		LogEditor _logEditor;

		enum class SceneState {
			Edit,
			Play,
		} _sceneState;

		bool _pause;

		Ref<Texture> _playButtonTex;
		Ref<Texture> _stopButtonTex;
		Ref<Texture> _pauseButtonTex;
		Ref<Texture> _resumeButtonTex;

		EditorCamera _camera;
		std::string _currentScenePath;
	};
}

