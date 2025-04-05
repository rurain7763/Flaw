#include "pch.h"
#include "Renderer2D.h"
#include "Graphics.h"

namespace flaw {
	struct VPMatrices {
		mat4 view = mat4(1.0f);
		mat4 projection = mat4(1.0f);
	};

	struct TexturedVertex {
		vec3 position;
		vec2 texcoord;
		vec4 color;
		uint32_t textureID;
		uint32_t id;
	};;

	constexpr uint32_t MaxBatchCount = 1000;

	static std::vector<TexturedVertex> g_quadVertices;
	static std::vector<TexturedVertex> g_textVertices;

	static Ref<VertexBuffer> g_vb;
	static Ref<IndexBuffer> g_ib;
	static Ref<ConstantBuffer> g_vpCBuff;
	static Ref<GraphicsPipeline> g_pipeline;
	static Ref<GraphicsPipeline> g_textPipeline;

	static VPMatrices g_mvp;

	static std::unordered_map<Ref<Texture2D>, uint32_t> g_quadTextures;
	static uint32_t g_quadIndexCount;

	static std::unordered_map<Ref<Texture2D>, uint32_t> g_textTextures;
	static uint32_t g_textIndexCount;

	void Renderer2D::Init() {
		auto& context = Graphics::GetGraphicsContext();

		g_quadVertices.resize(MaxBatchCount * 4);
		g_textVertices.resize(MaxBatchCount * 4);

		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Dynamic;
		vbDesc.elmSize = sizeof(TexturedVertex);
		vbDesc.bufferSize = sizeof(TexturedVertex) * MaxBatchCount * 4;
		g_vb = context.CreateVertexBuffer(vbDesc);

		std::vector<uint32_t> indices(MaxBatchCount * 6);
		for (uint32_t i = 0; i < MaxBatchCount; i++) {
			uint32_t offset = i * 4;
			indices[i * 6 + 0] = offset + 0;
			indices[i * 6 + 1] = offset + 1;
			indices[i * 6 + 2] = offset + 2;
			indices[i * 6 + 3] = offset + 0;
			indices[i * 6 + 4] = offset + 2;
			indices[i * 6 + 5] = offset + 3;
		}

		IndexBuffer::Descriptor ibDesc = {};
		ibDesc.usage = UsageFlag::Static;
		ibDesc.bufferSize = sizeof(uint32_t) * MaxBatchCount * 6;
		ibDesc.initialData = indices.data();

		g_ib = context.CreateIndexBuffer(ibDesc);

		g_vpCBuff = context.CreateConstantBuffer(sizeof(VPMatrices));

		Ref<GraphicsShader> shader = context.CreateGraphicsShader("Resources/Shaders/std2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<float>("COLOR", 4);
		shader->AddInputElement<uint32_t>("TEXTUREID", 1);
		shader->AddInputElement<uint32_t>("ID", 1);
		shader->CreateInputLayout();

		g_pipeline = context.CreateGraphicsPipeline();
		g_pipeline->SetShader(shader);
		g_pipeline->SetBlendMode(BlendMode::Alpha, true);
		g_pipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> textShader = context.CreateGraphicsShader("Resources/Shaders/text2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		textShader->AddInputElement<float>("POSITION", 3);
		textShader->AddInputElement<float>("TEXCOORD", 2);
		textShader->AddInputElement<float>("COLOR", 4);
		textShader->AddInputElement<uint32_t>("TEXTUREID", 1);
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
		g_vpCBuff.reset();
		g_pipeline.reset();
		g_textPipeline.reset();
	}

	void Renderer2D::Begin(const mat4& view, const mat4& projection) {
		g_mvp.view = view;
		g_mvp.projection = projection;
		g_vpCBuff->Update(&g_mvp, sizeof(VPMatrices));

		g_quadVertices.clear();
		g_quadTextures.clear();
		g_quadIndexCount = 0;

		g_textVertices.clear();
		g_textTextures.clear();
		g_textIndexCount = 0;
	}

	void Renderer2D::End() {
		auto& commandQueue = Graphics::GetCommandQueue();

		g_vb->Update(g_quadVertices.data(), sizeof(TexturedVertex), g_quadVertices.size());

		commandQueue.Begin();
		commandQueue.SetPipeline(g_pipeline);
		commandQueue.SetVertexBuffer(g_vb);
		commandQueue.SetConstantBuffer(g_vpCBuff, 0);
		for (auto& [texture, id] : g_quadTextures) {
			commandQueue.SetTexture(texture, id);
		}
		commandQueue.DrawIndexed(g_ib, g_quadIndexCount);
		commandQueue.End();

		commandQueue.Execute();

		g_vb->Update(g_textVertices.data(), sizeof(TexturedVertex), g_textVertices.size());

		commandQueue.Begin();
		commandQueue.SetPipeline(g_textPipeline);
		commandQueue.SetVertexBuffer(g_vb);
		commandQueue.SetConstantBuffer(g_vpCBuff, 0);

		for (auto& [texture, id] : g_textTextures) {
			commandQueue.SetTexture(texture, id);
		}

		commandQueue.DrawIndexed(g_ib, g_textIndexCount);
		commandQueue.End();

		commandQueue.Execute();
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const vec4& color) {
		uint32_t textureID = 0xFFFFFFFF;

		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f)), vec2(0.0f, 0.0f), color, textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, 0.5f, 0.0f, 1.0f)), vec2(1.0f, 0.0f), color, textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, -0.5f, 0.0f, 1.0f)), vec2(1.0f, 1.0f), color, textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f)), vec2(0.0f, 1.0f), color, textureID, id });

		g_quadIndexCount += 6;
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const Ref<Texture2D>& texture) {	
		uint32_t textureID;
		if (g_quadTextures.find(texture) == g_quadTextures.end()) {
			textureID = g_quadTextures.size();
			g_quadTextures[texture] = textureID;
		}
		else {
			textureID = g_quadTextures[texture];
		}

		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f)), vec2(0.0f, 0.0f), vec4(1.0f), textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, 0.5f, 0.0f, 1.0f)), vec2(1.0f, 0.0f), vec4(1.0f), textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, -0.5f, 0.0f, 1.0f)), vec2(1.0f, 1.0f), vec4(1.0f), textureID, id });
		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f)), vec2(0.0f, 1.0f), vec4(1.0f), textureID, id });

		g_quadIndexCount += 6;
	}

	void Renderer2D::DrawString(const uint32_t id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const Ref<Texture2D>& fontAtlas, const vec4& color) {
		float x = 0.0f;
		float y = 0.0f;
		float lineOffset = 0.0f;
		float kerningOffset = 0.0f;

		const float texelW = 1.0f / fontAtlas->GetWidth();
		const float texelH = 1.0f / fontAtlas->GetHeight();

		uint32_t atlasID;

		if (g_textTextures.find(fontAtlas) == g_textTextures.end()) {
			atlasID = g_textTextures.size();
			g_textTextures[fontAtlas] = atlasID;
		}
		else {
			atlasID = g_textTextures[fontAtlas];
		}

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

			g_textVertices.push_back({ vec3(transform * vec4(quadMin.x, quadMax.y, 0.0f, 1.0f)), vec2(texcoordMin.x, texcoordMax.y), color, atlasID, id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMax.x, quadMax.y, 0.0f, 1.0f)), vec2(texcoordMax.x, texcoordMax.y), color, atlasID, id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMax.x, quadMin.y, 0.0f, 1.0f)), vec2(texcoordMax.x, texcoordMin.y), color, atlasID, id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMin.x, quadMin.y, 0.0f, 1.0f)), vec2(texcoordMin.x, texcoordMin.y), color, atlasID, id });

			g_textIndexCount += 6;

			x += glyph.advance + kerningOffset;
		}
	}
}