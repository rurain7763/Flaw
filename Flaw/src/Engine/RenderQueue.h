#pragma once

#include "Core.h"
#include "Graphics.h"

#include <map>
#include <vector>

namespace flaw {
	struct InstancingObject {
		Ref<Mesh> mesh;
		std::vector<mat4> modelMatrices;
		uint32_t instanceCount = 0;
	};

	struct RenderEntry {
		Ref<Material> material;

		std::map<Ref<Mesh>, InstancingObject> instancingObjects;
		std::map<Ref<Mesh>, mat4> noBatchedObjects;
	};

	struct RenderQueue {
		std::vector<std::map<Ref<Material>, RenderEntry>> _renderEntries;

		RenderMode _currentRenderMode;
		std::map<Ref<Material>, RenderEntry>::iterator _currentRenderEntry;
		std::map<Ref<Material>, RenderEntry>::iterator _currentRenderEntryEnd;

		std::vector<Ref<VertexBuffer>> _vertexBufferPool;
		std::vector<Ref<IndexBuffer>> _indexBufferPool;

		RenderQueue();

		void Open();
		void Close();

		void Push(const Ref<Mesh>& mesh, const mat4& model, const Ref<Material>& material);
		void Pop();

		bool Empty();

		RenderEntry& Front();
	};
}