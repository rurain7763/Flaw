#pragma once

#include "Core.h"
#include "Graphics.h"
#include "Math/Math.h"
#include "Utils/Raycast.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	struct Vertex3D {
		vec3 position;
		vec2 texcoord;
		vec3 tangent;
		vec3 normal;
		vec3 binormal;
	};

	template<>
	struct Serializer<Vertex3D> {
		static void Serialize(SerializationArchive& archive, const Vertex3D& value) {
			archive << value.position.x << value.position.y << value.position.z;
			archive << value.texcoord.x << value.texcoord.y;
			archive << value.tangent.x << value.tangent.y << value.tangent.z;
			archive << value.normal.x << value.normal.y << value.normal.z;
			archive << value.binormal.x << value.binormal.y << value.binormal.z;
		}

		static void Deserialize(SerializationArchive& archive, Vertex3D& value) {
			archive >> value.position.x >> value.position.y >> value.position.z;
			archive >> value.texcoord.x >> value.texcoord.y;
			archive >> value.tangent.x >> value.tangent.y >> value.tangent.z;
			archive >> value.normal.x >> value.normal.y >> value.normal.z;
			archive >> value.binormal.x >> value.binormal.y >> value.binormal.z;
		}
	};

	struct SkinnedVertex3D {
		vec3 position;
		vec2 texcoord;
		vec3 tangent;
		vec3 normal;
		vec3 binormal;
		ivec4 boneIndices = ivec4(-1, -1, -1, -1);
		vec4 boneWeights = vec4(0.0f);
	};

	template<>
	struct Serializer<SkinnedVertex3D> {
		static void Serialize(SerializationArchive& archive, const SkinnedVertex3D& value) {
			archive << value.position.x << value.position.y << value.position.z;
			archive << value.texcoord.x << value.texcoord.y;
			archive << value.tangent.x << value.tangent.y << value.tangent.z;
			archive << value.normal.x << value.normal.y << value.normal.z;
			archive << value.binormal.x << value.binormal.y << value.binormal.z;
			archive << value.boneIndices.x << value.boneIndices.y << value.boneIndices.z << value.boneIndices.w;
			archive << value.boneWeights.x << value.boneWeights.y << value.boneWeights.z << value.boneWeights.w;
		}

		static void Deserialize(SerializationArchive& archive, SkinnedVertex3D& value) {
			archive >> value.position.x >> value.position.y >> value.position.z;
			archive >> value.texcoord.x >> value.texcoord.y;
			archive >> value.tangent.x >> value.tangent.y >> value.tangent.z;
			archive >> value.normal.x >> value.normal.y >> value.normal.z;
			archive >> value.binormal.x >> value.binormal.y >> value.binormal.z;
			archive >> value.boneIndices.x >> value.boneIndices.y >> value.boneIndices.z >> value.boneIndices.w;
			archive >> value.boneWeights.x >> value.boneWeights.y >> value.boneWeights.z >> value.boneWeights.w;
		}
	};

	struct MeshSegment {
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;

		uint32_t vertexStart = 0;
		uint32_t vertexCount = 0;
		uint32_t indexStart = 0;
		uint32_t indexCount = 0;
	};

	template<>
	struct Serializer<MeshSegment> {
		static void Serialize(SerializationArchive& archive, const MeshSegment& value) {
			archive << value.topology;
			archive << value.vertexStart;
			archive << value.vertexCount;
			archive << value.indexStart;
			archive << value.indexCount;
		}

		static void Deserialize(SerializationArchive& archive, MeshSegment& value) {
			archive >> value.topology;
			archive >> value.vertexStart;
			archive >> value.vertexCount;
			archive >> value.indexStart;
			archive >> value.indexCount;
		}
	};

	struct MeshBoundingSphere {
		vec3 center = vec3(0.0);
		float radius = 0.0f;
	};

	class Mesh {
	public:
		Mesh() = default;
		
		Mesh(PrimitiveTopology topology, const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) {
			_meshSegments.resize(1);
			_meshSegments[0].topology = topology;
			_meshSegments[0].vertexStart = 0;
			_meshSegments[0].vertexCount = vertices.size();
			_meshSegments[0].indexStart = 0;
			_meshSegments[0].indexCount = indices.size();

			GenerateGPUResources(vertices, indices);
			GenerateBVH(vertices, indices);
			GenerateBoundingSphere(vertices);
		}

		Mesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices, const std::vector<MeshSegment>& segments)
			: _meshSegments(segments)
		{
			GenerateGPUResources(vertices, indices);
			GenerateBVH(vertices, indices);
			GenerateBoundingSphere(vertices);
		}

		Mesh(PrimitiveTopology topology, const std::vector<SkinnedVertex3D>& vertices, const std::vector<uint32_t>& indices) {
			_meshSegments.resize(1);
			_meshSegments[0].topology = topology;
			_meshSegments[0].vertexStart = 0;
			_meshSegments[0].vertexCount = vertices.size();
			_meshSegments[0].indexStart = 0;
			_meshSegments[0].indexCount = indices.size();

			GenerateGPUResources(vertices, indices);
			GenerateBVH(vertices, indices);
			GenerateBoundingSphere(vertices);
		}

		Mesh(const std::vector<SkinnedVertex3D>& vertices, const std::vector<uint32_t>& indices, const std::vector<MeshSegment>& segments)
			: _meshSegments(segments)
		{
			GenerateGPUResources(vertices, indices);
			GenerateBVH(vertices, indices);
			GenerateBoundingSphere(vertices);
		}

		uint32_t GetMeshSegmentCount() const {
			return static_cast<uint32_t>(_meshSegments.size());
		}

		const std::vector<MeshSegment>& GetMeshSegments() const {
			return _meshSegments;
		}

		const MeshSegment& GetMeshSegementAt(int32_t index) const {
			return _meshSegments[index];
		}

		const MeshBoundingSphere& GetBoundingSphere() const {
			return _boundingSphere;
		}

		Ref<VertexBuffer> GetGPUVertexBuffer() const {
			return _gpuVertexBuffer;
		}

		Ref<IndexBuffer> GetGPUIndexBuffer() const {
			return _gpuIndexBuffer;
		}

		const std::vector<BVHNode>& GetBVHNodes() const {
			return _bvhNodes;
		}

		const std::vector<BVHTriangle>& GetBVHTriangles() const {
			return _bvhTriangles;
		}

	private:
		template<typename T>
		void GenerateBVH(const std::vector<T>& vertices, const std::vector<uint32_t>& indices) {
			if (vertices.empty()) {
				return;
			}

			_bvhTriangles.clear();
			_bvhNodes.clear();

			for (const auto& meshInfo : _meshSegments) {
				std::vector<BVHNode> bvhNodes;
				std::vector<BVHTriangle> bvhTriangles;

				Raycast::BuildBVH(
					[&vertices, &indices, meshInfo](int32_t index) { return vertices[meshInfo.vertexStart + indices[meshInfo.indexStart + index]].position; },
					meshInfo.indexCount,
					bvhNodes,
					bvhTriangles
				);

				_bvhNodes.insert(_bvhNodes.end(), bvhNodes.begin(), bvhNodes.end());
				_bvhTriangles.insert(_bvhTriangles.end(), bvhTriangles.begin(), bvhTriangles.end());
			}
		}

		template<typename T>
		void GenerateBoundingSphere(const std::vector<T>& vertices) {
			if (vertices.empty()) {
				return;
			}

			vec3 min = vertices[0].position;
			vec3 max = vertices[0].position;
			for (const auto& vertex : vertices) {
				min = glm::min(min, vertex.position);
				max = glm::max(max, vertex.position);
			}

			_boundingSphere.center=  (min + max) * 0.5f;

			_boundingSphere.radius = 0.0f;
			for (const auto& vertex : vertices) {
				float distance = glm::length(vertex.position - _boundingSphere.center);
				_boundingSphere.radius = std::max(_boundingSphere.radius, distance);
			}
		}

		template<typename T>
		void GenerateGPUResources(const std::vector<T>& vertices, const std::vector<uint32_t>& indices) {
			auto& graphicsContext = Graphics::GetGraphicsContext();

			VertexBuffer::Descriptor vertexDesc = {};
			vertexDesc.usage = UsageFlag::Static;
			vertexDesc.elmSize = sizeof(T);
			vertexDesc.bufferSize = vertices.size() * sizeof(T);
			vertexDesc.initialData = vertices.data();
			_gpuVertexBuffer = graphicsContext.CreateVertexBuffer(vertexDesc);

			IndexBuffer::Descriptor indexDesc = {};
			indexDesc.usage = UsageFlag::Static;
			indexDesc.bufferSize = indices.size() * sizeof(uint32_t);
			indexDesc.initialData = indices.data();
			_gpuIndexBuffer = graphicsContext.CreateIndexBuffer(indexDesc);
		}

	private:
		std::vector<MeshSegment> _meshSegments;

		// NOTE: may be we need cpu vertex buffer and index buffer

		Ref<VertexBuffer> _gpuVertexBuffer;
		Ref<IndexBuffer> _gpuIndexBuffer;

		std::vector<BVHNode> _bvhNodes;
		std::vector<BVHTriangle> _bvhTriangles;

		MeshBoundingSphere _boundingSphere;
	};
}