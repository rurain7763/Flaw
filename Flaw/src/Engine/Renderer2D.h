#pragma once

#include "Graphics/GraphicsContext.h"
#include "Font/Font.h"
#include "Math/Math.h"

namespace flaw {
	class Renderer2D {
	public:
		Renderer2D(GraphicsContext& context);

		void Begin(const mat4& view, const mat4& projection);
		void End();

		void DrawQuad(const uint32_t id, const mat4& transform, const vec4& color);
		void DrawQuad(const uint32_t id, const mat4& transform, const Ref<Texture>& texture);

		void DrawString(const uint32_t id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const vec4& color);

	private:
		GraphicsContext& _context;
		GraphicsCommandQueue& _commandQueue;

		struct MVPMatrices {
			mat4 model = mat4(1.0f);
			mat4 view = mat4(1.0f);
			mat4 projection = mat4(1.0f);
		};

		struct ConstDatas {
			int32_t textureEnabled;
			int32_t padding[3];
			vec4 color;
		};

		struct Vertex {
			vec3 position;
			vec2 texcoord;
			uint32_t id;
		};

		Vertex _defaultQuadVertices[4] = {
			{ vec3(-0.5f, 0.5f, 0.0f), vec2(0.0f, 0.0f), 0xffffffff },
			{ vec3(0.5f, 0.5f, 0.0f), vec2(1.0f, 0.0f), 0xffffffff },
			{ vec3(0.5f, -0.5f, 0.0f), vec2(1.0f, 1.0f), 0xffffffff },
			{ vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 1.0f), 0xffffffff }
		};

		Ref<VertexBuffer> _vb;
		Ref<IndexBuffer> _ib;
		Ref<ConstantBuffer> _mvpCBuff;
		Ref<ConstantBuffer> _constCBuff;
		Ref<GraphicsPipeline> _pipeline;
		Ref<GraphicsPipeline> _textPipeline;

		MVPMatrices _mvp;
		ConstDatas _constDatas;
	};
}