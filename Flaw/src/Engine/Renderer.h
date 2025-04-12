#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"

namespace flaw {
	struct RenderEnvironment {
		mat4 view;
		mat4 projection;

		// Lights
		SkyLight skyLight;
		std::vector<DirectionalLight> directionalLights;
		std::vector<PointLight> pointLights;
		std::vector<SpotLight> spotLights;
	};

	class Renderer {
	public:
		static void Init();
		static void Cleanup();

		static void Begin(const RenderEnvironment& env);
		static void End();

		static void DrawCube(const uint32_t id, const mat4& transform);
		static void DrawCube(const uint32_t id, const mat4& transform, const Material& material);

		static void DrawSphere(const uint32_t id, const mat4& transform);
		static void DrawSphere(const uint32_t id, const mat4& transform, const Material& material);
	};
}