#include "pch.h"
#include "Application.h"
#include "Platform.h"
#include "Input/Input.h"
#include "Time/Time.h"
#include "Platform/PlatformEvents.h"
#include "Graphics/DX11/DXContext.h"
#include "Debug/Instrumentor.h"
#include "Log/Log.h"
#include "Scripting.h"
#include "Project.h"
#include "AssetManager.h"
#include "Graphics.h"
#include "Fonts.h"

namespace flaw {
	Application::Application(const ApplicationProps& props) {
		FLAW_PROFILE_FUNCTION();

		if (props.argc > 0) {
			std::filesystem::path execPath = props.argv[0];
			std::filesystem::current_path(execPath.parent_path());
		}

		if (props.argc > 1) {
			std::filesystem::path projPath = props.argv[1];
			Project::FromFile(projPath.string().c_str());
		}

		{
			FLAW_PROFILE_SCOPE("Initialize Platform");
			Platform::Init(props.title, props.width, props.height, _eventDispatcher);
		}

		{
			FLAW_PROFILE_SCOPE("Initialize Graphics");
			Graphics::Init(GraphicsType::DX11);
			Renderer2D::Init();
		}

		{
			FLAW_PROFILE_SCOPE("Initialize Fonts");
			Fonts::Init();
		}

		{
			FLAW_PROFILE_SCOPE("Initialize Asset Manager");
			AssetManager::Init();
		}

		{
			FLAW_PROFILE_SCOPE("Initialize Mono Scripting");
			Scripting::Init(*this);
		}

		// 이벤트 등록
		_eventDispatcher.Register<KeyPressEvent>([this](const KeyPressEvent& event) { Input::OnKeyPress(event.key); }, PID(this));
		_eventDispatcher.Register<KeyReleaseEvent>([this](const KeyReleaseEvent& event) { Input::OnKeyRelease(event.key); }, PID(this));
		_eventDispatcher.Register<MouseMoveEvent>([this](const MouseMoveEvent& event) { Input::OnMouseMove(event.x, event.y); }, PID(this));
		_eventDispatcher.Register<MousePressEvent>([this](const MousePressEvent& event) { Input::OnMousePress(event.button); }, PID(this));
		_eventDispatcher.Register<MouseReleaseEvent>([this](const MouseReleaseEvent& event) { Input::OnMouseRelease(event.button); }, PID(this));
		_eventDispatcher.Register<MouseScrollEvent>([this](const MouseScrollEvent& event) { Input::OnMouseScroll(event.xOffset, event.yOffset); }, PID(this));
		_eventDispatcher.Register<WindowResizeEvent>([this](const WindowResizeEvent& event) { 
			Graphics::Resize(event.frameBufferWidth, event.frameBufferHeight); 
			Graphics::SetViewport(0, 0, event.frameBufferWidth, event.frameBufferHeight);
		}, PID(this));
		_eventDispatcher.Register<WindowIconifyEvent>([this](const WindowIconifyEvent& event) { _minimized = event.iconified; }, PID(this));
		_eventDispatcher.Register<WindowFocusEvent>([this](const WindowFocusEvent& event) { if (!event.focused) { Input::Reset(); } }, PID(this));

		_running = true;
		_minimized = false;
	}

	Application::~Application() {
		_eventDispatcher.UnregisterAll(PID(this));

		Scripting::Cleanup();
		AssetManager::Cleanup();
		Fonts::Cleanup();
		Renderer2D::Cleanup();
		Graphics::Cleanup();
		Platform::Cleanup();
	}

	void Application::PushLayer(Layer* layer) {
		_layerRegistry.PushLayer(layer);
		layer->OnAttatch();
	}

	void Application::PushLayerAsOverlay(Layer* layer) {
		_layerRegistry.PushLayerAsOverlay(layer);
		layer->OnAttatch();
	}

	void Application::RemoveLayer(Layer* layer) {
		_layerRegistry.RemoveLayer(layer);
		layer->OnDetach();
	}

	void Application::AddTask(const std::function<void()>& task) {
		std::lock_guard<std::mutex> lock(_taskMutex);
		_tasks.emplace_back(task);
	}

	void Application::Run() {
		Time::Start();

		while (_running && Platform::PollEvents()) {
			_eventDispatcher.PollEvents();

			Time::Update();

			if (!_minimized) {
				Graphics::Prepare();

				for (Layer* layer : _layerRegistry) {
					layer->OnUpdate();
				}
			}

			Input::Update();

			ExecuteTasks();
		}
	}

	void Application::ExecuteTasks() {
		std::vector<std::function<void()>> tasksToExecute;

		{
			std::lock_guard<std::mutex> lock(_taskMutex);
			std::swap(tasksToExecute, _tasks);  // 기존 큐와 빈 큐를 교체
		}

		for (auto& task : tasksToExecute) {
			task();
		}
	}
}