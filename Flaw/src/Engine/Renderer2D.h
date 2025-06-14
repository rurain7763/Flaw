#pragma once

#include "Math/Math.h"
#include "Graphics/Texture.h"
#include "Graphics/GraphicsBuffers.h"
#include "Graphics/GraphicsPipeline.h"
#include "Font/Font.h"
#include "ECS/ECS.h"

namespace flaw {
	class Renderer2D {
	public:
		static void Init();
		static void Cleanup();

		static void Begin(const mat4& view, const mat4& projection);
		static void End();

		static void DrawQuad(const entt::entity id, const mat4& transform, const vec4& color);
		static void DrawQuad(const entt::entity id, const mat4& transform, const Ref<Texture2D>& texture);

		static void DrawCircle(const entt::entity id, const mat4& transform, const vec4& color, const float thickness = 1.0);

		// TODO: add thickness
		static void DrawLine(const entt::entity id, const vec3& start, const vec3& end, const vec4& color);

		static void DrawLineRect(const entt::entity id, const mat4& transform, const vec4& color);

		static void DrawString(const entt::entity id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const Ref<Texture2D>& fontAtlas, const vec4& color);
	};
}