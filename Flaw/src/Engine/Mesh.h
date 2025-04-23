#pragma once

#include "Core.h"
#include "Graphics/GraphicsContext.h"
#include "Math/Math.h"
#include "Raycast.h"

namespace flaw {
	struct QuadVertex {
		vec3 position;
		vec2 texcoord;
		vec4 color;
		uint32_t textureID;
		uint32_t id;
	};

	struct CircleVertex {
		vec3 localPosition;
		vec3 worldPosition;
		float thickness;
		vec4 color;
		uint32_t id;
	};

	struct LineVertex {
		vec3 position;
		vec4 color;
		uint32_t id;
	};

	struct PointVertex {
		vec3 position;
	};

	struct Vertex3D {
		vec3 position;
		vec2 texcoord;
		vec3 tangent;
		vec3 normal;
		vec3 binormal;
	};

	struct Mesh {
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;

		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;

		std::vector<BVHNode> bvhNodes;
		std::vector<BVHTriangle> bvhTriangles;

		vec3 boundingSphereCenter = vec3(0.0);
		float boundingSphereRadius = 0;

		void GenerateBVH();
		void GenerateBoundingSphere();
	};
}