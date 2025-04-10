#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>

using namespace glm;

namespace flaw {
	constexpr vec3 Forward = vec3(0.0f, 0.0f, 1.0f);
	constexpr vec3 Backward = vec3(0.0f, 0.0f, -1.0f);
	constexpr vec3 Right = vec3(1.0f, 0.0f, 0.0f);
	constexpr vec3 Left = vec3(-1.0f, 0.0f, 0.0f);
	constexpr vec3 Up = vec3(0.0f, 1.0f, 0.0f);
	constexpr vec3 Down = vec3(0.0f, -1.0f, 0.0f);

	inline mat4 Translate(const vec3& translation) {
		return translate(mat4(1.0f), translation);
	}

	inline mat4 QRotate(const vec3& rotation) {
		return toMat4(glm::quat(rotation));
	}

	inline vec3 QRotate(const vec3& rotation, const vec3& axis) {
		return rotate(glm::quat(rotation), axis);
	}

	inline mat4 Scale(const vec3& scale) {
		return glm::scale(mat4(1.0f), scale);
	}

	inline mat4 ModelMatrix(const vec3& position, const vec3& rotation, const vec3& scale) {
		return Translate(position) * QRotate(rotation) * Scale(scale);
	}

	inline void ExtractModelMatrix(const mat4& matrix, vec3& outPosition, vec3& outRotation, vec3& outScale) {
		// 위치 추출
		outPosition.x = matrix[3][0];
		outPosition.y = matrix[3][1];
		outPosition.z = matrix[3][2];

		// 스케일 추출
		outScale.x = length(vec3(matrix[0][0], matrix[0][1], matrix[0][2]));
		outScale.y = length(vec3(matrix[1][0], matrix[1][1], matrix[1][2]));
		outScale.z = length(vec3(matrix[2][0], matrix[2][1], matrix[2][2]));

		// 스케일 제거 후 회전 추출
		mat4 rotationMatrix = matrix;
		rotationMatrix[0] /= outScale.x;
		rotationMatrix[1] /= outScale.y;
		rotationMatrix[2] /= outScale.z;

		outRotation = eulerAngles(toQuat(rotationMatrix));
	}

	inline mat4 LookAt(const vec3& position, const vec3& target, const vec3& up) {
		return glm::lookAt(position, target, up);
	}

	inline mat4 ViewMatrix(const vec3& position, const vec3& rotation) {
		return LookAt(position, position + QRotate(rotation, Forward), Up);
	}

	inline mat4 Orthographic(float left, float right, float bottom, float top, float nearClip, float farClip) {
		return ortho(left, right, bottom, top, nearClip, farClip);
	}

	inline mat4 Perspective(float fov, float aspectRatio, float nearClip, float farClip) {
		return glm::perspective(fov, aspectRatio, nearClip, farClip);
	}

	// Screen Space to Viewport Space(NDC: -1 ~ 1)
	inline vec3 ScreenToViewport(const vec2& screenPos, const vec4& viewport) {
		return vec3(
			(2.0f * (screenPos.x - viewport.x)) / viewport.z - 1.0f,
			-((2.0f * (screenPos.y - viewport.y)) / viewport.w - 1.0f),
			0.0f
		);
	}

	// Final: Screen Space to World Space
	inline vec3 ScreenToWorld(const vec2& screenPos, const vec4& viewport, const mat4& projectionMat, const mat4& viewMat) {
		// 1. Screen → Viewport (NDC)
		vec3 viewportPos = ScreenToViewport(screenPos, viewport);

		// 2. Viewport → Projection
		vec4 projectionPos = glm::inverse(projectionMat) * vec4(viewportPos, 1.0f);
		projectionPos /= projectionPos.w;
		
		return glm::inverse(viewMat) * projectionPos;
	}

	inline bool GetIntersectionPos(const vec3& rayOrigin, const vec3& rayDir, const float rayLength, const vec3& planeNormal, const vec3& planePos, vec3& outPos) {
		// check 2 points are both outside or inside the plane
		vec3 startPoint = rayOrigin;
		vec3 endPoint = rayOrigin + rayDir * rayLength;

		const float dotStart = dot(planeNormal, startPoint - planePos);
		const float dotEnd = dot(planeNormal, endPoint - planePos);

		if (dotStart * dotEnd > 0.0f) {
			// both points are outside or inside the plane
			return false;
		}

		const float t = dotStart / (dotStart - dotEnd);

		outPos = glm::mix(startPoint, endPoint, t);

		return true;
	}

	inline bool IsInside(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p) {
		vec3 v0 = p2 - p1;
		vec3 v1 = p3 - p1;
		vec3 v2 = p - p1;

		// Barycentric coordinates 계산
		float d00 = dot(v0, v0);
		float d01 = dot(v0, v1);
		float d11 = dot(v1, v1);
		float d20 = dot(v2, v0);
		float d21 = dot(v2, v1);

		float denom = d00 * d11 - d01 * d01;
		float v = (d11 * d20 - d01 * d21) / denom;
		float w = (d00 * d21 - d01 * d20) / denom;
		float u = 1.0f - v - w;

		// Barycentric 좌표 값이 모두 0~1 범위에 있어야 삼각형 내부에 있음
		return (u >= 0.0f) && (v >= 0.0f) && (w >= 0.0f);
	}

	inline vec4 CalculateViewport(const vec2& contentSize, const vec2& actualSize) {
		vec2 size = contentSize;
		vec2 offset = vec2(0.0f);

		if (size.x > size.y) {
			const float ratio = actualSize.x / actualSize.y;
			size.x = size.y * ratio;
			offset.x += (contentSize.x - size.x) * 0.5f;
		}
		else {
			const float ratio = actualSize.y / actualSize.x;
			size.y = size.x * ratio;
			offset.y += (contentSize.y - size.y) * 0.5f;
		}

		return vec4(offset.x, offset.y, size.x, size.y);
	}

	inline bool EpsilonEqual(float a, float b, float epsilon = 0.00001f) {
		return abs(a - b) < epsilon;
	}

	template <typename T>
	inline T Remap(T min, T max, T value, T targetMin, T targetMax) {
		if (min == max) return targetMin; // 예외 처리
		using FloatT = std::conditional_t<std::is_integral_v<T>, double, T>;
		FloatT t = static_cast<FloatT>(value - min) / static_cast<FloatT>(max - min);
		t = glm::clamp(t, static_cast<FloatT>(0), static_cast<FloatT>(1)); // 범위 제한
		return static_cast<T>(targetMin + t * (targetMax - targetMin));
	}
}
