#include "pch.h"
#include "LayerRegistry.h"

namespace flaw {
	void LayerRegistry::PushLayer(Layer* layer) {
		_layers.insert(_layers.begin() + _insertionIndex, layer);
		++_insertionIndex;
	}

	void LayerRegistry::PushLayerAsOverlay(Layer* layer) {
		_layers.push_back(layer);
	}

	void LayerRegistry::RemoveLayer(Layer* layer) {
		for (auto it = _layers.begin(); it != _layers.end(); ++it) {
			if (*it == layer) {
				_layers.erase(it);
				--_insertionIndex;
				break;
			}
		}
	}

	void LayerRegistry::Clear() {
		_layers.clear();
	}
}