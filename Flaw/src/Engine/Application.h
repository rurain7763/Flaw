#pragma once

#include "Core.h"
#include "LayerRegistry.h"
#include "Event/EventDispatcher.h"
#include "Utils/ThreadPool.h"

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

		// add main thread task, task will be excuted after everthing is done
		void AddTask(const std::function<void()>& task);

		// add async task, task will be excuted in thread pool
		void AddAsyncTask(const std::function<void()>& task);

		void Run();

		inline EventDispatcher& GetEventDispatcher() { return _eventDispatcher; }

	private:
		void ExecuteTasks();

	private:
		EventDispatcher _eventDispatcher;
		LayerRegistry _layerRegistry;

		bool _running;
		bool _minimized;

		ThreadPool _threadPool;

		std::vector<std::function<void()>> _tasks;
		std::mutex _taskMutex;
	};
}

