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

	void RenderQueue::Push(const Ref<Mesh>& mesh, const mat4& model, const Ref<Material>& material) {
		auto& entry = _renderEntries[uint32_t(material->renderMode)][material];
		entry.material = material;

		// �̹� Instancing ���̸� �׳� �߰�
		auto& instancingMap = entry.instancingObjects;
		auto itInst = instancingMap.find(mesh);
		if (itInst != instancingMap.end()) {
			itInst->second.modelMatrices.push_back(model);
			itInst->second.instanceCount++;
			return;
		}

		// ������ �Ϲ� ��ο쿡 ������ -> Instancing���� �°�
		auto itNoBatch = entry.noBatchedObjects.find(mesh);
		if (itNoBatch != entry.noBatchedObjects.end()) {
			InstancingObject instance;
			instance.mesh = mesh;
			instance.modelMatrices.push_back(itNoBatch->second); // ���� ��
			instance.modelMatrices.push_back(model);             // �� ��
			instance.instanceCount = 2;
			instancingMap[mesh] = std::move(instance);
			entry.noBatchedObjects.erase(itNoBatch); // �Ϲ� ��ο쿡�� ����
			return;
		}

		// ó�� �� �޽ø� �ϴ� �Ϲ� ��ο�� ���
		entry.noBatchedObjects[mesh] = model;
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