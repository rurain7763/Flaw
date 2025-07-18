#include "DebugRender.h"

namespace flaw {
    void DebugRender::DrawCube(const mat4& transform, const vec3& color) {
        // front
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(0.0f, 0.0f, -0.5), vec3(0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

		// back
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(0.0f, 0.0f, 0.5), vec3(0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

		// left
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(-0.5, 0.0f, 0.0f), vec3(0.0, -glm::half_pi<float>(), 0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

		// right
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(0.5, 0.0f, 0.0), vec3(0.0, glm::half_pi<float>(), 0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

		// top
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(0.0f, 0.5, 0.0f), vec3(-glm::half_pi<float>(), 0.0, 0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

		// bottom
		Renderer2D::DrawLineRect(
			entt::null,
			transform * ModelMatrix(vec3(0.0f, -0.5, 0.0f), vec3(glm::half_pi<float>(), 0.0, 0.0), vec3(1.0)),
			vec4(color, 1.0)
		);

        // front line
        vec3 front = normalize(mat3(transform) * Forward);
		Renderer2D::DrawLine(
			entt::null,
			ExtractPosition(transform),
			ExtractPosition(transform) + front * 0.5f,
			vec4(color, 1.0)
		);
    }

	void DebugRender::DrawSphere(const mat4& transform, const float radius, const vec3& color) {
        Renderer2D::DrawCircle(
            entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(color, 1.0),
            0.02f
        );

        Renderer2D::DrawCircle(
            entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(glm::half_pi<float>(), 0.0, 0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(color, 1.0),
            0.02f
        );

        Renderer2D::DrawCircle(
            entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(0.0, glm::half_pi<float>(), 0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(color, 1.0),
            0.02f
        );
	}

    void DebugRender::DrawCone(const mat4& transform, const float height, const float outer, const float inner, const vec3& color) {
        float outterRadius = height * tan(outer);

        Renderer2D::DrawCircle(
            entt::null,
            transform * ModelMatrix(vec3(0.0, 0.0, height), vec3(0.0), vec3(outterRadius * 2, outterRadius * 2, 0.0)),
            vec4(color, 1.0),
            0.02f
        );

        float innerRadius = height * tan(inner);

        Renderer2D::DrawCircle(
            entt::null,
            transform * ModelMatrix(vec3(0.0, 0.0, height), vec3(0.0), vec3(innerRadius * 2, innerRadius * 2, 0.0)),
            vec4(color, 1.0),
            0.02f
        );

        vec3 front = mat3(transform) * Forward;
		vec3 right = normalize(cross(Up, front));
		vec3 up = normalize(cross(front, right));
		vec3 worldPosition = ExtractPosition(transform);
        vec3 circlePosition = worldPosition + front * height;

        vec3 topPos = circlePosition + up * outterRadius;
        vec3 rightPos = circlePosition + right * outterRadius;
        vec3 leftPos = circlePosition + -right * outterRadius;
        vec3 bottomPos = circlePosition + -up * outterRadius;

        Renderer2D::DrawLine(entt::null, worldPosition, topPos, vec4(color, 1.0));
        Renderer2D::DrawLine(entt::null, worldPosition, leftPos, vec4(color, 1.0));
        Renderer2D::DrawLine(entt::null, worldPosition, rightPos, vec4(color, 1.0));
        Renderer2D::DrawLine(entt::null, worldPosition, bottomPos, vec4(color, 1.0));
    }

    void DebugRender::DrawFrustum(const Frustum& frustrum, const mat4& transform, const vec3& color) {
		Frustum::Corners corners = frustrum.GetCorners();

		// near plane
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topLeftNear, 1.0), transform * vec4(corners.topRightNear, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topRightNear, 1.0), transform * vec4(corners.bottomRightNear, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomRightNear, 1.0), transform * vec4(corners.bottomLeftNear, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomLeftNear, 1.0), transform * vec4(corners.topLeftNear, 1.0), vec4(color, 1.0));

		// far plane
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topLeftFar, 1.0), transform * vec4(corners.topRightFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topRightFar, 1.0), transform * vec4(corners.bottomRightFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomRightFar, 1.0), transform * vec4(corners.bottomLeftFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomLeftFar, 1.0), transform * vec4(corners.topLeftFar, 1.0), vec4(color, 1.0));

		// connecting lines
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topLeftNear, 1.0), transform * vec4(corners.topLeftFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.topRightNear, 1.0), transform * vec4(corners.topRightFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomRightNear, 1.0), transform * vec4(corners.bottomRightFar, 1.0), vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, transform * vec4(corners.bottomLeftNear, 1.0), transform * vec4(corners.bottomLeftFar, 1.0), vec4(color, 1.0));
	}

	void DebugRender::DrawLineTriangle(const mat4& transform, const vec3& p0, const vec3& p1, const vec3& p2, const vec3& color) {
		vec3 worldP0 = transform * vec4(p0, 1.0);
		vec3 worldP1 = transform * vec4(p1, 1.0);
		vec3 worldP2 = transform * vec4(p2, 1.0);

		Renderer2D::DrawLine(entt::null, worldP0, worldP1, vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, worldP1, worldP2, vec4(color, 1.0));
		Renderer2D::DrawLine(entt::null, worldP2, worldP0, vec4(color, 1.0));
	}
}
