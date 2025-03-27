#pragma once

#include "Core.h"

#include <vector>
#include <stdint.h>

namespace flaw {
	class FAPI Layer {
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnAttatch() {}
		virtual void OnUpdate() {}
		virtual void OnDetach() {}
		virtual void OnRender() {}
	};

	class FAPI LayerRegistry {
	public:
		void PushLayer(Layer* layer);
		void PushLayerAsOverlay(Layer* layer);
		void RemoveLayer(Layer* layer);
		void Clear();

		inline std::vector<Layer*>::iterator begin() { return _layers.begin(); }
		inline std::vector<Layer*>::iterator end() { return _layers.end(); }

	private:
		int32_t _insertionIndex = 0;
		std::vector<Layer*> _layers;
	};
}