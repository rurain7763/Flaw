#pragma once

#include <Flaw.h>

namespace flaw {
	class DebugRender {
	public:
		static void DrawCube(const mat4& transform, const vec3& color);
		static void DrawSphere(const mat4& transform, const float radius, const vec3& color);
		static void DrawCone(const mat4& transform, const float height, const float outer, const float inner, const vec3& color);
		static void DrawFrustum(const Frustum& frustrum, const mat4& transform, const vec3& color);
		static void DrawLineTriangle(const mat4& transform, const vec3& p0, const vec3& p1, const vec3& p2, const vec3& color);
	};
}