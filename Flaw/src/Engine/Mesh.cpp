#include "pch.h"
#include "Mesh.h"

namespace flaw {
	void Mesh::GenerateBVH() {
		if (vertices.empty()) {
			return;
		}

		bvhTriangles.clear();
		bvhNodes.clear();

		Raycast::BuildBVH(
			[this](int32_t index) { return vertices[indices[index]].position; },
			indices.size(),
			bvhNodes,
			bvhTriangles
		);
	}

	void Mesh::GenerateBoundingSphere() {
		if (vertices.empty()) {
			return;
		}

		vec3 min = vertices[0].position;
		vec3 max = vertices[0].position;
		for (const auto& vertex : vertices) {
			min = glm::min(min, vertex.position);
			max = glm::max(max, vertex.position);
		}

		boundingSphereCenter = (min + max) * 0.5f;

		boundingSphereRadius = 0.0f;
		for (const auto& vertex : vertices) {
			float distance = glm::length(vertex.position - boundingSphereCenter);
			boundingSphereRadius = std::max(boundingSphereRadius, distance);
		}
	}
}