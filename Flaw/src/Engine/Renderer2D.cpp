#include "pch.h"
#include "Renderer2D.h"
#include "Graphics.h"

namespace flaw {
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

	struct TexturedVertex {
		vec3 position;
		vec2 texcoord;
		uint32_t id;
	};

	static TexturedVertex g_defaultQuadVertices[4] = {
		{ vec3(-0.5f, 0.5f, 0.0f), vec2(0.0f, 0.0f), 0xffffffff },
		{ vec3(0.5f, 0.5f, 0.0f), vec2(1.0f, 0.0f), 0xffffffff },
		{ vec3(0.5f, -0.5f, 0.0f), vec2(1.0f, 1.0f), 0xffffffff },
		{ vec3(-0.5f, -0.5f, 0.0f), vec2(0.0f, 1.0f), 0xffffffff }
	};

	static Ref<VertexBuffer> g_vb;
	static Ref<IndexBuffer> g_ib;
	static Ref<ConstantBuffer> g_mvpCBuff;
	static Ref<ConstantBuffer> g_constCBuff;
	static Ref<GraphicsPipeline> g_pipeline;
	static Ref<GraphicsPipeline> g_textPipeline;

	static MVPMatrices g_mvp;
	static ConstDatas g_constDatas;

	void Renderer2D::Init() {
		auto& context = Graphics::GetGraphicsContext();

		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Dynamic;
		vbDesc.elmSize = sizeof(TexturedVertex);
		vbDesc.count = 4;
		g_vb = context.CreateVertexBuffer(vbDesc);

		g_ib = context.CreateIndexBuffer();
		const uint32_t indices[] = { 0, 1, 2, 0, 2, 3 };
		g_ib->Update(indices, 6);

		g_mvpCBuff = context.CreateConstantBuffer(sizeof(MVPMatrices));
		g_constCBuff = context.CreateConstantBuffer(sizeof(ConstDatas));

		Ref<GraphicsShader> shader = context.CreateGraphicsShader("Resources/Shaders/std2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<uint32_t>("ID", 1);
		shader->CreateInputLayout();

		g_pipeline = context.CreateGraphicsPipeline();
		g_pipeline->SetShader(shader);
		g_pipeline->SetBlendMode(BlendMode::Alpha, true);
		g_pipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> textShader = context.CreateGraphicsShader("Resources/Shaders/text2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		textShader->AddInputElement<float>("POSITION", 3);
		textShader->AddInputElement<float>("TEXCOORD", 2);
		textShader->AddInputElement<uint32_t>("ID", 1);
		textShader->CreateInputLayout();

		g_textPipeline = context.CreateGraphicsPipeline();
		g_textPipeline->SetShader(textShader);
		g_textPipeline->SetBlendMode(BlendMode::Alpha, true);
		g_textPipeline->SetCullMode(CullMode::None);
	}

	void Renderer2D::Cleanup() {
		g_vb.reset();
		g_ib.reset();
		g_mvpCBuff.reset();
		g_constCBuff.reset();
		g_pipeline.reset();
		g_textPipeline.reset();
	}

	void Renderer2D::Begin(const mat4& view, const mat4& projection) {
		g_mvp.view = view;
		g_mvp.projection = projection;
	}

	void Renderer2D::End() {
		// TODO: 최적화를 위해 쿼드를 여기서 그려야 함
		// 현 상태는 드로우 호출 시 마다 쿼드를 그림
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const vec4& color) {
		auto& commandQueue = Graphics::GetCommandQueue();

		for (uint32_t i = 0; i < 4; i++) {
			g_defaultQuadVertices[i].id = id;
		}

		g_vb->Update(g_defaultQuadVertices, sizeof(TexturedVertex), 4);
		
		g_mvp.model = transform;

		g_constDatas.textureEnabled = 0;
		g_constDatas.color = color;

		g_mvpCBuff->Update(&g_mvp, sizeof(MVPMatrices));
		g_constCBuff->Update(&g_constDatas, sizeof(ConstDatas));

		commandQueue.Begin();
		commandQueue.SetPipeline(g_pipeline);
		commandQueue.SetVertexBuffer(g_vb);
		commandQueue.SetConstantBuffer(g_mvpCBuff, 0);
		commandQueue.SetConstantBuffer(g_constCBuff, 1);
		commandQueue.DrawIndexed(g_ib);
		commandQueue.End();

		commandQueue.Execute();
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const Ref<Texture2D>& texture) {
		auto& commandQueue = Graphics::GetCommandQueue();

		for (uint32_t i = 0; i < 4; i++) {
			g_defaultQuadVertices[i].id = id;
		}

		g_vb->Update(g_defaultQuadVertices, sizeof(TexturedVertex), 4);
		
		g_mvp.model = transform;

		g_constDatas.textureEnabled = 1;
		g_constDatas.color = vec4(1.0f);

		g_mvpCBuff->Update(&g_mvp, sizeof(MVPMatrices));
		g_constCBuff->Update(&g_constDatas, sizeof(ConstDatas));

		commandQueue.Begin();
		commandQueue.SetPipeline(g_pipeline);
		commandQueue.SetVertexBuffer(g_vb);
		commandQueue.SetConstantBuffer(g_mvpCBuff, 0);
		commandQueue.SetConstantBuffer(g_constCBuff, 1);
		commandQueue.SetTexture(texture, 0);
		commandQueue.DrawIndexed(g_ib);
		commandQueue.End();

		commandQueue.Execute();
	}

	void Renderer2D::DrawString(const uint32_t id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const Ref<Texture2D>& fontAtlas, const vec4& color) {
		auto& commandQueue = Graphics::GetCommandQueue();

		g_mvp.model = transform;

		g_constDatas.textureEnabled = 1;
		g_constDatas.color = color;

		g_mvpCBuff->Update(&g_mvp, sizeof(MVPMatrices));
		g_constCBuff->Update(&g_constDatas, sizeof(ConstDatas));
		
		float x = 0.0f;
		float y = 0.0f;
		float lineOffset = 0.0f;
		float kerningOffset = 0.0f;

		const float texelW = 1.0f / fontAtlas->GetWidth();
		const float texelH = 1.0f / fontAtlas->GetHeight();

		FontGlyph glyph;
		for (int32_t i = 0; i < text.size(); ++i) {
			uint32_t uniChar = text[i];

			if (uniChar == L'\r') {
				continue;
			}

			if (uniChar == L'\n') {
				x = 0.0f;
				y -= font->LineHeight() - lineOffset;
				continue;
			}

			if (uniChar == L'\t') {
				if (font->TryGetGlyph(' ', glyph)) {
					x += (glyph.advance + kerningOffset) * 4;
				}
				continue;
			}

			if (uniChar == L' ') {
				if (font->TryGetGlyph(' ', glyph)) {
					x += glyph.advance + kerningOffset;
				}
				continue;
			}

			if (!font->TryGetGlyph(uniChar, glyph)) {
				if (!font->TryGetGlyph('?', glyph)) {
					continue;
				}
			}

			vec2 texcoordMin = vec2(glyph.tl, glyph.tb);
			vec2 texcoordMax = vec2(glyph.tr, glyph.tt);

			texcoordMin *= vec2(texelW, texelH);
			texcoordMax *= vec2(texelW, texelH);

			vec2 quadMin = vec2(glyph.l, glyph.b);
			vec2 quadMax = vec2(glyph.r, glyph.t);

			quadMin += vec2(x, y); quadMax += vec2(x, y);

			// rendering
			TexturedVertex vertices[4] = {
				{ vec3(quadMin.x, quadMax.y, 0.0f), vec2(texcoordMin.x, texcoordMax.y), id },
				{ vec3(quadMax.x, quadMax.y, 0.0f), vec2(texcoordMax.x, texcoordMax.y), id },
				{ vec3(quadMax.x, quadMin.y, 0.0f), vec2(texcoordMax.x, texcoordMin.y), id },
				{ vec3(quadMin.x, quadMin.y, 0.0f), vec2(texcoordMin.x, texcoordMin.y), id }
			};

			g_vb->Update(vertices, sizeof(TexturedVertex), 4);

			commandQueue.Begin();
			commandQueue.SetPipeline(g_textPipeline);
			commandQueue.SetVertexBuffer(g_vb);
			commandQueue.SetConstantBuffer(g_mvpCBuff, 0);
			commandQueue.SetConstantBuffer(g_constCBuff, 1);
			commandQueue.SetTexture(fontAtlas, 0);
			commandQueue.DrawIndexed(g_ib);
			commandQueue.End();

			commandQueue.Execute();

			x += glyph.advance + kerningOffset;
		}
	}
}