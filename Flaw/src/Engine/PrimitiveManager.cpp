#include "pch.h"
#include "PrimitiveManager.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	static Ref<Mesh> g_cubeMesh;
	static Ref<Mesh> g_sphereMesh;

	void PrimitiveManager::Init() {
		CreateCubeMesh();
		CreateSphereMesh();
	}

	void PrimitiveManager::CreateCubeMesh() {
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;

		GenerateCube([&vertices](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				vertices.push_back(vertex);
			},
			indices
		);

		g_cubeMesh = CreateRef<Mesh>(PrimitiveTopology::TriangleList, vertices, indices);
	}

	void PrimitiveManager::CreateSphereMesh() {
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;

		GenerateSphere([&vertices](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				vertices.push_back(vertex);
			},
			indices, 20, 20
		);

		g_sphereMesh = CreateRef<Mesh>(PrimitiveTopology::TriangleList, vertices, indices);
	}

	void PrimitiveManager::Cleanup() {
		g_cubeMesh.reset();
		g_sphereMesh.reset();
	}

	Ref<Mesh> PrimitiveManager::GetCubeMesh() {
		return g_cubeMesh;
	}

	Ref<Mesh> PrimitiveManager::GetSphereMesh() {
		return g_sphereMesh;
	}
}