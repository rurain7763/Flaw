#include "pch.h"
#include "RenderQueue.h"

namespace flaw {
	RenderQueue::RenderQueue() {
		_renderEntries.resize(uint32_t(RenderMode::Count));
	}

	void RenderQueue::Open() {
		_currentRenderMode = RenderMode::Count;
		_currentRenderEntry = {};
		_currentRenderEntryEnd = {};
		for (int32_t i = 0; i < _renderEntries.size(); i++) {
			_renderEntries[i].clear();
		}
	}

	void RenderQueue::Close() {
		_currentRenderMode = RenderMode::Count;
		for (int32_t i = 0; i < _renderEntries.size(); i++) {
			if (_renderEntries[i].size() > 0) {
				_currentRenderMode = (RenderMode)i;
				_currentRenderEntry = _renderEntries[i].begin();
				_currentRenderEntryEnd = _renderEntries[i].end();
				break;
			}
		}
	}

	void RenderQueue::Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material) {
		auto& entry = _renderEntries[uint32_t(material->renderMode)][material];
		entry.material = material;

		MeshKey meshKey(mesh, segmentIndex);

		auto it = entry.instancingObjects.find(meshKey);
		if (it != entry.instancingObjects.end()) {
			it->second.modelMatrices.push_back(worldMat);
			it->second.instanceCount++;
		}
		else {
			InstancingObject instance;
			instance.mesh = mesh;
			instance.segmentIndex = segmentIndex;
			instance.modelMatrices.push_back(worldMat);
			instance.instanceCount = 1;

			entry.instancingObjects[meshKey] = instance;
		}
	}

	void RenderQueue::Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material) {
		int32_t segmentIdx = 0;
		for (auto& segment : mesh->GetMeshSegments()) {
			Push(mesh, segmentIdx, worldMat, material);
			segmentIdx++;
		}
	}

	void RenderQueue::Pop() {
		FASSERT(_currentRenderMode != RenderMode::Count, "RenderQueue is empty");
		if (++_currentRenderEntry == _currentRenderEntryEnd) {
			do {
				_currentRenderMode = (RenderMode)(uint32_t(_currentRenderMode) + 1);
				if (_currentRenderMode == RenderMode::Count) {
					break;
				}
				_currentRenderEntry = _renderEntries[uint32_t(_currentRenderMode)].begin();
				_currentRenderEntryEnd = _renderEntries[uint32_t(_currentRenderMode)].end();
			} while (_currentRenderEntry == _currentRenderEntryEnd);
		}
	}

	bool RenderQueue::Empty() {
		return _currentRenderMode == RenderMode::Count;
	}

	RenderEntry& RenderQueue::Front() {
		FASSERT(_currentRenderMode != RenderMode::Count, "RenderQueue is empty");
		return _currentRenderEntry->second;
	}
}