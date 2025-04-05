#pragma once

#include "Core.h"
#include "Renderer2D.h"
#include "LayerRegistry.h"
#include "Event/EventDispatcher.h"
#include "Platform/PlatformContext.h"
#include "Graphics/GraphicsContext.h"

#include <functional>
#include <mutex>

namespace flaw {
	struct ApplicationProps {
		const char* title;
		int32_t width;
		int32_t height;

		int argc;
		char** argv;
	};

	class FAPI Application {
	public:
		Application(const ApplicationProps& props);
		virtual ~Application();

		void PushLayer(Layer* layer);
		void PushLayerAsOverlay(Layer* layer);
		void RemoveLayer(Layer* layer);

		void AddTask(const std::function<void()>& task);

		void Run();

		inline EventDispatcher& GetEventDispatcher() { return _eventDispatcher; }

	private:
		void ExecuteTasks();

	private:
		EventDispatcher _eventDispatcher;
		LayerRegistry _layerRegistry;

		bool _running;
		bool _minimized;

		std::vector<std::function<void()>> _tasks;
		std::mutex _taskMutex;
	};
}

