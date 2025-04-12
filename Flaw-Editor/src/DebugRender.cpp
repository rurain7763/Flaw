#include "DebugRender.h"

namespace flaw {
	void DebugRender::DrawSphere(const mat4& transform, const float radius, const vec3& color) {
        Renderer2D::DrawCircle(
            (uint32_t)entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(0.0, 1.0, 0.0, 1.0),
            0.02f
        );

        Renderer2D::DrawCircle(
            (uint32_t)entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(glm::half_pi<float>(), 0.0, 0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(0.0, 1.0, 0.0, 1.0),
            0.02f
        );

        Renderer2D::DrawCircle(
            (uint32_t)entt::null,
            transform * ModelMatrix(vec3(0.0f, 0.0f, 0.0f), vec3(0.0, glm::half_pi<float>(), 0.0), vec3(radius * 2.0f, radius * 2.0f, 1.0)),
            vec4(0.0, 1.0, 0.0, 1.0),
            0.02f
        );
	}

    void DebugRender::DrawCone(const mat4& transform, const float height, const float outer, const float inner, const vec3& color) {
        float outterRadius = height * tan(outer);

        Renderer2D::DrawCircle(
            (uint32_t)entt::null,
            transform * ModelMatrix(vec3(0.0, 0.0, height), vec3(0.0), vec3(outterRadius * 2, outterRadius * 2, 0.0)),
            vec4(0.0, 1.0, 0.0, 1.0),
            0.02f
        );

        float innerRadius = height * tan(inner);

        Renderer2D::DrawCircle(
            (uint32_t)entt::null,
            transform * ModelMatrix(vec3(0.0, 0.0, height), vec3(0.0), vec3(innerRadius * 2, innerRadius * 2, 0.0)),
            vec4(0.0, 1.0, 0.0, 1.0),
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

        Renderer2D::DrawLine((uint32_t)entt::null, worldPosition, topPos, vec4(0.0, 1.0, 0.0, 1.0));
        Renderer2D::DrawLine((uint32_t)entt::null, worldPosition, leftPos, vec4(0.0, 1.0, 0.0, 1.0));
        Renderer2D::DrawLine((uint32_t)entt::null, worldPosition, rightPos, vec4(0.0, 1.0, 0.0, 1.0));
        Renderer2D::DrawLine((uint32_t)entt::null, worldPosition, bottomPos, vec4(0.0, 1.0, 0.0, 1.0));
    }
}
