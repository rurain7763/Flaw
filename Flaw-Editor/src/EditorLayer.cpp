#include "EditorLayer.h"
#include "EditorEvents.h"

#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>
#include "Editor/ImGuizmo.h"

namespace flaw {
    EditorLayer::EditorLayer(flaw::Application& app) 
	    : _app(app)
		, _graphicsContext(app.GetGraphicsContext())
		, _outlinerEditor(app)
		, _viewportEditor(app, _camera)
		, _contentBrowserEditor(app)
		, _detailsEditor(app)
		, _sceneState(SceneState::Edit)
		, _pause(false)
    {
        PlatformContext& platformContext = app.GetPlatformContext();

        int32_t frameBufferWidth, frameBufferHeight;
        platformContext.GetFrameBufferSize(frameBufferWidth, frameBufferHeight);
    }

    void EditorLayer::OnAttatch() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

        // setup fonts
		io.Fonts->AddFontFromFileTTF("Resources/Fonts/OpenSans/OpenSans-Bold.ttf", 18.0f);
		io.FontDefault = io.Fonts->AddFontFromFileTTF("Resources/Fonts/OpenSans/OpenSans-Regular.ttf", 18.0f);

		SetTheme(EditorTheme::Dark);

        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        auto* wndContext = static_cast<flaw::WindowsContext*>(&_app.GetPlatformContext());
        auto* dxContext = static_cast<flaw::DXContext*>(&_app.GetGraphicsContext());

        ImGui_ImplWin32_Init(wndContext->GetWindowHandle());
        ImGui_ImplDX11_Init(dxContext->Device().Get(), dxContext->DeviceContext().Get());

		wndContext->SetUserWndProc(ImGui_ImplWin32_WndProcHandler);

        CreateRequiredTextures();

        auto projConfig = Project::GetConfig();
		if (projConfig.startScene.empty()) {
			_editorScene = CreateRef<Scene>(_app);
		    _outlinerEditor.SetScene(_editorScene);
            _viewportEditor.SetScene(_editorScene);
		}
		else {
			const std::string fullPath = projConfig.path + "/" + projConfig.startScene;
			OpenScene(fullPath.c_str());
		}
    }

    void EditorLayer::OnDetach() {
	    ImGui_ImplDX11_Shutdown();
	    ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void EditorLayer::OnUpdate() {
        switch (_sceneState) {
        case SceneState::Edit:
			RenderTargetScene(_editorScene);
            break;
        case SceneState::Play:
			if (!_pause) {
				_runtimeScene->OnUpdate();
			}
            else {
				RenderTargetScene(_runtimeScene);
            }
            break;
        }

	    ImGui_ImplDX11_NewFrame();
	    ImGui_ImplWin32_NewFrame();

	    ImGui::NewFrame();
        ImGuizmo::BeginFrame();

        {
            static bool open = true;
            static bool opt_fullscreen = true;
            static bool opt_padding = false;
            static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
            ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
            if (opt_fullscreen)
            {
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);
                ImGui::SetNextWindowViewport(viewport->ID);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
            }
            else
            {
                dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
            }

            if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
                window_flags |= ImGuiWindowFlags_NoBackground;

            if (!opt_padding)
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			ImGuiStyle& style = ImGui::GetStyle();

            ImGui::Begin("DockSpace Demo", &open, window_flags);

            if (!opt_padding)
                ImGui::PopStyleVar();
            if (opt_fullscreen)
                ImGui::PopStyleVar(2);

            ImGuiIO& io = ImGui::GetIO();
			const float backupSizeX = style.WindowMinSize.x;
			style.WindowMinSize.x = 370.0f;
            if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
            {
                ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
                ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
            }
			style.WindowMinSize.x = backupSizeX;

            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_FILE_PATH")) {
                    std::filesystem::path path = (const char*)payload->Data;
                    if (path.extension() == ".scene") {
					    OpenScene(path.generic_string().c_str());
                    }
                }
                ImGui::EndDragDropTarget();
            }

            if (ImGui::BeginMenuBar()) {
				if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open Project", "Ctrl+O")) {
						OpenProject();
                    }

					ImGui::Separator();

					if (ImGui::MenuItem("New Scene", "Ctrl+N")) {
                        NewScene();
                    }

					if (ImGui::MenuItem("Save Scene", "Ctrl+S")) {
						SaveScene();
                    }

					if (ImGui::MenuItem("Save Scene As..")) {
						SaveSceneAs();
                    }

					ImGui::EndMenu();
				}

                ImGui::EndMenuBar();
            }

            if (Input::GetKey(KeyCode::LCtrl) && Input::GetKey(KeyCode::N)) {
				NewScene();
            }

            if (Input::GetKey(KeyCode::LCtrl) && Input::GetKey(KeyCode::O)) {
				OpenScene();
            }

            if (Input::GetKey(KeyCode::LCtrl) && Input::GetKey(KeyCode::S)) {
				SaveScene();
            }

            ImGui::End();
        }

        {
            ImGui::Begin("Status");
	        ImGui::Text("FPS: %d", flaw::Time::FPS());
            ImGui::End();
        }

        OnRenderToolbar();

        _outlinerEditor.OnRender();
		_viewportEditor.OnRender();
		_contentBrowserEditor.OnRender();
		_detailsEditor.OnRender();
        _logEditor.OnRender();

	    ImGui::Render();

	    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	    ImGuiIO& io = ImGui::GetIO(); (void)io;
	    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		    ImGui::UpdatePlatformWindows();
		    ImGui::RenderPlatformWindowsDefault();
	    }

		_graphicsContext.Present();
    }

    void EditorLayer::SetTheme(EditorTheme theme) {
        ImGuiStyle& style = ImGui::GetStyle();

        switch (theme) {
        case EditorTheme::Orange:
        {
            // 배경색(더 어두운 주황 계열)
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.07f, 0.03f, 1.00f);

            // 타이틀 바
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.60f, 0.25f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.70f, 0.30f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.45f, 0.20f, 0.00f, 1.00f);

            // 버튼 색상 (더 어두운 톤)
            style.Colors[ImGuiCol_Button] = ImVec4(0.55f, 0.25f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.70f, 0.35f, 0.05f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.80f, 0.40f, 0.10f, 1.00f);

            // 프레임 및 위젯 색상
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.10f, 0.05f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.15f, 0.07f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.35f, 0.20f, 0.10f, 1.00f);

            // 슬라이더, 체크박스 등 인터랙션 요소
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.70f, 0.35f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.80f, 0.40f, 0.05f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.75f, 0.35f, 0.00f, 1.00f);

            // 테두리 및 포커스 컬러
            style.Colors[ImGuiCol_Border] = ImVec4(0.50f, 0.25f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.20f, 0.10f, 0.02f, 1.00f);

            // 툴팁 및 팝업
            style.Colors[ImGuiCol_PopupBg] = ImVec4(0.20f, 0.12f, 0.05f, 1.00f);

            // 리스트 및 선택 항목
            style.Colors[ImGuiCol_Header] = ImVec4(0.60f, 0.25f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.30f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.80f, 0.35f, 0.05f, 1.00f);

            // 모서리 둥글기 및 테두리 크기 조정
            style.FrameRounding = 4.0f;
            style.WindowRounding = 5.0f;
            style.GrabRounding = 3.0f;
            style.FrameBorderSize = 1.2f;
            break;
        }
        case EditorTheme::Dark:
        {
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
            break;
        }
        case EditorTheme::Light:
        {
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.85f, 0.85f, 1.00f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.75f, 0.75f, 0.75f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            break;
        }
        case EditorTheme::GrayOrange: // 회색 + 주황 테마
        {
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.15f, 0.15f, 0.15f, 1.00f); // 전체 배경 (짙은 회색)

            // 타이틀 바 (주황색으로 변경)
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.85f, 0.45f, 0.00f, 1.00f); // 주황색
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.00f); // 활성화된 타이틀바 주황색
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.75f, 0.40f, 0.00f, 1.00f); // 축소된 타이틀바 주황색

            // 버튼 (회색 톤이지만 강조는 주황색)
            style.Colors[ImGuiCol_Button] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.85f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.00f);

            // 프레임 및 위젯 색상
            style.Colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.85f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.00f);

            // 슬라이더, 체크박스 등 인터랙션 요소
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.85f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.00f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.90f, 0.50f, 0.00f, 1.00f);

            // 테두리 및 포커스 컬러
            style.Colors[ImGuiCol_Border] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);

            // 리스트 및 선택 항목
            style.Colors[ImGuiCol_Header] = ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.85f, 0.45f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.00f);

            // 둥글기 및 테두리 크기 조정
            style.FrameRounding = 5.0f;
            style.WindowRounding = 6.0f;
            style.GrabRounding = 4.0f;
            style.FrameBorderSize = 1.0f;
            break;
        }
        }
    }

    void EditorLayer::CreateRequiredTextures() {
        Texture::Descriptor desc = {};

        Image playImg("Resources/Icons/Play.png");

		desc.width = playImg.Width();
		desc.height = playImg.Height();
		desc.format = PixelFormat::RGBA8;
		desc.data = playImg.Data().data();
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		_playButtonTex = _graphicsContext.CreateTexture2D(desc);

		Image stopImg("Resources/Icons/Stop.png");

		desc.width = stopImg.Width();
		desc.height = stopImg.Height();
		desc.data = stopImg.Data().data();

		_stopButtonTex = _graphicsContext.CreateTexture2D(desc);

		Image pauseImg("Resources/Icons/Pause.png");

		desc.width = pauseImg.Width();
		desc.height = pauseImg.Height();
		desc.data = pauseImg.Data().data();

		_pauseButtonTex = _graphicsContext.CreateTexture2D(desc);

		Image resumeImg("Resources/Icons/Resume.png");

		desc.width = resumeImg.Width();
		desc.height = resumeImg.Height();
		desc.data = resumeImg.Data().data();

		_resumeButtonTex = _graphicsContext.CreateTexture2D(desc);
    }

    void EditorLayer::RenderTargetScene(const Ref<Scene>& scene) {
        auto& renderer2D = _app.GetRenderer2D();
        auto& enttRegistry = scene->GetRegistry();

        // draw sprite
        renderer2D.Begin(_camera.GetViewMatrix(), _camera.GetProjectionMatrix());

        for (auto&& [entity, transComp, sprComp] : enttRegistry.view<TransformComponent, SpriteRendererComponent>().each()) {

            if (sprComp.texture == nullptr) {
                renderer2D.DrawQuad((uint32_t)entity, transComp.GetTransform(), sprComp.color);
            }
            else {
                renderer2D.DrawQuad((uint32_t)entity, transComp.GetTransform(), sprComp.texture);
            }
        }

		for (auto&& [entity, transComp, textComp] : enttRegistry.view<TransformComponent, TextComponent>().each()) {
			if (textComp.font) {
			    renderer2D.DrawString((uint32_t)entity, transComp.GetTransform(), textComp.text, textComp.font, textComp.color);
			}
		}

        renderer2D.End();

        _camera.OnUpdate();
    }

    void EditorLayer::OnRenderToolbar() {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar |
                                        ImGuiWindowFlags_NoResize |
                                        ImGuiWindowFlags_NoScrollbar |
                                        ImGuiWindowFlags_NoCollapse;

        ImGui::Begin("##Toolbar", nullptr, window_flags);

        ImGuiDockNode* dockNode = ImGui::GetCurrentWindow()->DockNode;
        if (dockNode) {
            dockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;  // 명시적으로 탭 바 숨김
        }

        // Play/Stop 이미지 버튼
		Ref<Texture> currentTexture = _sceneState == SceneState::Play ? _stopButtonTex : _playButtonTex;
        if (_pause) {
			currentTexture = _resumeButtonTex;
        }

		Ref<DXTexture2D> dxTexture = std::static_pointer_cast<DXTexture2D>(currentTexture);

		const ImVec2 buttonSizeEach = ImVec2(24, 24);

		if (ImGui::ImageButton(
            "##SceneStateButton",
            (ImTextureID)dxTexture->GetShaderResourceView().Get(), 
            buttonSizeEach))
        {
			if (_sceneState == SceneState::Edit) {
				OnScenePlay();
			}
			else if (_sceneState == SceneState::Play) {
				if (_pause) {
					OnSceneResume();
				}
                else {
                    OnSceneStop();
                }
			}
		}

		if (_sceneState != SceneState::Edit && !_pause) {
			ImGui::SameLine();

			Ref<DXTexture2D> pauseTexture = std::static_pointer_cast<DXTexture2D>(_pauseButtonTex);
			if (ImGui::ImageButton(
				"##PauseButton",
				(ImTextureID)pauseTexture->GetShaderResourceView().Get(),
				buttonSizeEach))
			{
                OnScenePause();
			}
		}

        ImGui::End();

        ImGui::PopStyleVar(3);
    }

	void EditorLayer::NewScene() {
		if (_sceneState == SceneState::Play) {
			OnSceneStop();
		}

		_editorScene = CreateRef<Scene>(_app);
		_outlinerEditor.SetScene(_editorScene);
		_viewportEditor.SetScene(_editorScene);
		_currentScenePath = "";
	}

	void EditorLayer::SaveScene() {
		if (_currentScenePath.empty()) {
			std::string filePath = FileDialogs::SaveFile(_app.GetPlatformContext(), "Scene Files (*.scene)\0*.scene\0");
			if (!filePath.empty()) {
				_editorScene->ToFile(filePath.c_str());
			}
		}
		else {
			_editorScene->ToFile(_currentScenePath.c_str());
		}
	}

	void EditorLayer::SaveSceneAs() {
		std::string filePath = FileDialogs::SaveFile(_app.GetPlatformContext(), "Scene Files (*.scene)\0*.scene\0");
		if (!filePath.empty()) {
			_editorScene->ToFile(filePath.c_str());
			_currentScenePath = filePath;
		}
	}

	void EditorLayer::OpenScene() {
		std::string filePath = FileDialogs::OpenFile(_app.GetPlatformContext(), "Scene Files (*.scene)\0*.scene\0");
		if (!filePath.empty()) {
			OpenScene(filePath.c_str());
		}
	}

	void EditorLayer::OpenScene(const char* path) {
		if (_sceneState != SceneState::Edit) {
			OnSceneStop();
		}

		_editorScene = CreateRef<Scene>(_app);
		_editorScene->FromFile(path);
		_outlinerEditor.SetScene(_editorScene);
		_viewportEditor.SetScene(_editorScene);
		_currentScenePath = path;
	}

	void EditorLayer::OpenProject() {
		std::string filePath = FileDialogs::OpenFile(_app.GetPlatformContext(), "Project Files (*.fproj)\0*.fproj\0");
		if (!filePath.empty()) {
			Project::FromFile(filePath.c_str());

			auto projectConfig = Project::GetConfig();
			if (!projectConfig.startScene.empty()) {
			    OpenScene(projectConfig.startScene.c_str());
			}
		}
	}

	void EditorLayer::OnScenePlay() {
		_runtimeScene = _editorScene->Clone();
		_runtimeScene->OnStart();
		_outlinerEditor.SetScene(_runtimeScene);
		_viewportEditor.SetScene(_runtimeScene);

		_sceneState = SceneState::Play;

        _app.GetEventDispatcher().Dispatch<OnSceneStateChangeEvent>(_sceneState);
	}

    void EditorLayer::OnScenePause() {
		_pause = true;

		_app.GetEventDispatcher().Dispatch<OnScenePauseEvent>(_pause);
    }

    void EditorLayer::OnSceneResume() {
        _pause = false;

		_app.GetEventDispatcher().Dispatch<OnScenePauseEvent>(_pause);
    }

    void EditorLayer::OnSceneStop() {
		_runtimeScene->OnEnd();
		_runtimeScene = nullptr;

		_outlinerEditor.SetScene(_editorScene);
		_viewportEditor.SetScene(_editorScene);

		_sceneState = SceneState::Edit;

		_app.GetEventDispatcher().Dispatch<OnSceneStateChangeEvent>(_sceneState);
    }
}

