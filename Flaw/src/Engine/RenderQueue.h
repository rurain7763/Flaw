#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Mesh.h"
#include "Material.h"
#include "Skeleton.h"

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

	struct SkeletalInstancingObject {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;

		mat4 modelMatrices;
		Ref<StructuredBuffer> skeletonBoneMatrices;
	};

	struct RenderEntry {
		Ref<Material> material;

		std::map<MeshKey, int32_t> instancintIndexMap;
		std::vector<InstancingObject> instancingObjects;
		std::vector<SkeletalInstancingObject> skeletalInstancingObjects;
	};

	class RenderQueue {
	public:
		RenderQueue();

		void Open();
		void Close();

		void Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material);
		void Push(const Ref<Mesh>& mesh, int segmentIndex, const mat4& worldMat, const Ref<Material>& material, const Ref<StructuredBuffer>& boneMatrices);
		void Push(const Ref<Mesh>& mesh, const mat4& worldMat, const Ref<Material>& material);
		void Pop();

		bool Empty();

		RenderEntry& Front();

	private:
		int32_t GetRenderEntryIndex(const Ref<Material>& material);

	private:
		std::map<Ref<Material>, int32_t> _materialIndexMap;
		std::vector<std::vector<RenderEntry>> _renderEntries;

		RenderMode _currentRenderMode;
		std::vector<RenderEntry>::iterator _currentRenderEntry;
		std::vector<RenderEntry>::iterator _currentRenderEntryEnd;
	};
}