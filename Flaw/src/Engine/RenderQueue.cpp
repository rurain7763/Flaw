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

		_materialIndexMap.clear();
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

	int32_t RenderQueue::GetRenderEntryIndex(const Ref<Material>& material) {
		int32_t entryIndex = -1;

		auto indexIt = _materialIndexMap.find(material);
		if (indexIt == _materialIndexMap.end()) {
			auto& entryList = _renderEntries[uint32_t(material->renderMode)];
			entryIndex = entryList.size();
			_materialIndexMap[material] = entryIndex;
			entryList.resize(entryIndex + 1);
		}
		else {
			entryIndex = indexIt->second;
		}

		return entryIndex;
	}

	void RenderQueue::Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material) {
		auto& entryList = _renderEntries[uint32_t(material->renderMode)];
		int32_t entryIndex = GetRenderEntryIndex(material);
		
		auto& entry = entryList[entryIndex];
		entry.material = material;

		MeshKey meshKey(mesh, segmentIndex);
		int32_t instanceIndex = -1;

		auto instancingIndexIt = entry.instancintIndexMap.find(meshKey);
		if (instancingIndexIt == entry.instancintIndexMap.end()) {
			InstancingObject instance;
			instance.mesh = mesh;
			instance.segmentIndex = segmentIndex;
			instance.instanceCount = 0;

			instanceIndex = entry.instancingObjects.size();
			entry.instancintIndexMap[meshKey] = instanceIndex;
			entry.instancingObjects.emplace_back(instance);
		}
		else {
			instanceIndex = instancingIndexIt->second;
		}

		auto& instance = entry.instancingObjects[instanceIndex];
		instance.modelMatrices.emplace_back(worldMat);
		instance.instanceCount++;
	}

	void RenderQueue::Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices) {
		auto& entryList = _renderEntries[uint32_t(material->renderMode)];
		int32_t entryIndex = GetRenderEntryIndex(material);

		auto& entry = entryList[entryIndex];
		entry.material = material;

		SkeletalInstancingObject instance = {};
		instance.mesh = mesh;
		instance.segmentIndex = segmentIndex;
		instance.modelMatrices = worldMat;
		instance.skeletonBoneMatrices = boneMatrices;

		entry.skeletalInstancingObjects.emplace_back(instance);
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
		return *_currentRenderEntry;
	}
}