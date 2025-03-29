#pragma once
#if false

#include "GraphicsData.h"
#include "GraphicsType.h"
#include "Buffers.h"

namespace flaw {
	class Graphics;

	class Mesh {
	public:
		Mesh() = default;
		Mesh(Ref<VertexBuffer> vertexBuffer, Ref<IndexBuffer> indexBuffer) : _vertexBuffer(vertexBuffer), _indexBuffer(indexBuffer) {}
		
		template<typename T>
		Mesh(const MeshData<T>& meshData) {
			Initialize(meshData.vertices, meshData.indices);
		}

		template<typename T>
		void Initialize(const std::vector<T>& vertices, const std::vector<uint32_t>& indices) {
			if constexpr (
				std::is_same_v<T, TexturedVertex> ||
				std::is_same_v<T, ColoredVertex> ||
				std::is_same_v<T, BasicVertex> ||
				std::is_same_v<T, TexcoordVertex>
			) {
				_vertexBuffer = Graphics::CreateVertexBuffer();
				_vertexBuffer->Update(vertices.data(), vertices.size(), sizeof(T));
			}
			else {
				static_assert(std::is_same_v<T, void>, "Invalid vertex type");
			}

			_indexBuffer = Graphics::CreateIndexBuffer();
			_indexBuffer->Update(indices);
		}

		FINLINE void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) { _primitiveTopology = primitiveTopology; }
		FINLINE Ref<VertexBuffer> GetVertexBuffer() const { return _vertexBuffer; }
		FINLINE Ref<IndexBuffer> GetIndexBuffer() const { return _indexBuffer; }

		FINLINE PrimitiveTopology GetPrimitiveTopology() const { return _primitiveTopology; }

	private:
		PrimitiveTopology _primitiveTopology = PrimitiveTopology::TriangleList;

		Ref<VertexBuffer> _vertexBuffer;
		Ref<IndexBuffer> _indexBuffer;
	};
}
#endif