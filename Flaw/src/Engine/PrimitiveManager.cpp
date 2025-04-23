#include "pch.h"
#include "PrimitiveManager.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	static Ref<Mesh> g_cubeMesh;
	static Ref<Mesh> g_sphereMesh;

	void PrimitiveManager::Init() {
		// cube mesh
		g_cubeMesh = CreateRef<Mesh>();

		GenerateCube([](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				g_cubeMesh->vertices.push_back(vertex);
			},
			g_cubeMesh->indices
		);

		g_cubeMesh->GenerateBVH();
		g_cubeMesh->GenerateBoundingSphere();

		// sphere mesh 
		g_sphereMesh = CreateRef<Mesh>();

		GenerateSphere([](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				g_sphereMesh->vertices.push_back(vertex);
			},
			g_sphereMesh->indices, 20, 20
		);

		g_sphereMesh->GenerateBVH();
		g_sphereMesh->GenerateBoundingSphere();
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