#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Mesh.h"

#include <map>
#include <vector>

namespace flaw {
	using MeshKey = std::pair<Ref<Mesh>, int32_t>;

	struct InstancingObject {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;

		std::vector<mat4> modelMatrices;
		uint32_t instanceCount = 0;
	};

	struct RenderEntry {
		Ref<Material> material;
		std::map<MeshKey, InstancingObject> instancingObjects;
	};

	struct RenderQueue {
		std::vector<std::map<Ref<Material>, RenderEntry>> _renderEntries;

		RenderMode _currentRenderMode;
		std::map<Ref<Material>, RenderEntry>::iterator _currentRenderEntry;
		std::map<Ref<Material>, RenderEntry>::iterator _currentRenderEntryEnd;

		RenderQueue();

		void Open();
		void Close();

		void Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material);
		void Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material);
		void Pop();

		bool Empty();

		RenderEntry& Front();
	};
}