#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Mesh.h"
#include "Material.h"
#include "Skeleton.h"

#include <map>
#include <vector>

namespace flaw {
	struct MeshKey {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;

		bool operator==(const MeshKey& other) const {
			return mesh == other.mesh && segmentIndex == other.segmentIndex;
		};
	};
	
	struct SkeletalMeshKey {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;
		Ref<StructuredBuffer> skeletonBoneMatrices;

		bool operator==(const SkeletalMeshKey& other) const {
			return mesh == other.mesh && segmentIndex == other.segmentIndex && skeletonBoneMatrices == other.skeletonBoneMatrices;
		}
	};
}

namespace std {
	template<>
	struct hash<flaw::MeshKey> {
		size_t operator()(const flaw::MeshKey& key) const {
			return hash<flaw::Ref<flaw::Mesh>>()(key.mesh) ^ hash<int32_t>()(key.segmentIndex);
		}
	};

	template <>
	struct hash<flaw::SkeletalMeshKey> {
		size_t operator()(const flaw::SkeletalMeshKey& key) const {
			return hash<flaw::Ref<flaw::Mesh>>()(key.mesh) ^ hash<int32_t>()(key.segmentIndex) ^ hash<flaw::Ref<flaw::StructuredBuffer>>()(key.skeletonBoneMatrices);
		}
	};
}

namespace flaw {
	struct InstancingObject {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;

		std::vector<mat4> modelMatrices;
		uint32_t instanceCount = 0;
	};

	struct SkeletalInstancingObject {
		Ref<Mesh> mesh;
		int32_t segmentIndex = 0;

		std::vector<mat4> modelMatrices;
		Ref<StructuredBuffer> skeletonBoneMatrices;
		uint32_t instanceCount = 0;
	};

	struct RenderEntry {
		Ref<Material> material;

		std::unordered_map<MeshKey, int32_t> instancintIndexMap;
		std::vector<InstancingObject> instancingObjects;

		std::unordered_map<SkeletalMeshKey, int32_t> skeletalInstancingIndexMap;
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
		std::unordered_map<Ref<Material>, int32_t> _materialIndexMap;
		std::vector<std::vector<RenderEntry>> _renderEntries;

		RenderMode _currentRenderMode;
		std::vector<RenderEntry>::iterator _currentRenderEntry;
		std::vector<RenderEntry>::iterator _currentRenderEntryEnd;
	};
}

