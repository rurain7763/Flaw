#include "pch.h"
#include "Renderer2D.h"
#include "Graphics.h"
#include "Math/Math.h"
#include "Time/Time.h"

namespace flaw {
	constexpr uint32_t MaxBatchCount = 1000;

	static std::vector<QuadVertex> g_quadVertices;
	static std::vector<CircleVertex> g_circleVertices;
	static std::vector<LineVertex> g_lineVertices;
	static std::vector<QuadVertex> g_textVertices;

	static Ref<VertexBuffer> g_quadVB;
	static Ref<VertexBuffer> g_circleVB;
	static Ref<VertexBuffer> g_lineVB;
	static Ref<VertexBuffer> g_textVB;
	static Ref<IndexBuffer> g_ib;
	static Ref<ConstantBuffer> g_vpCB;
	static Ref<ConstantBuffer> g_globalCB;
	static Ref<GraphicsPipeline> g_quadPipeline;
	static Ref<GraphicsPipeline> g_circlePipeline;
	static Ref<GraphicsPipeline> g_linePipeline;
	static Ref<GraphicsPipeline> g_textPipeline;

	static std::unordered_map<Ref<Texture2D>, uint32_t> g_quadTextures;
	static uint32_t g_quadIndexCount;

	static uint32_t g_circleIndexCount;

	static uint32_t g_lineIndexCount;

	static std::unordered_map<Ref<Texture2D>, uint32_t> g_textTextures;
	static uint32_t g_textIndexCount;

	void Renderer2D::Init() {
		auto& context = Graphics::GetGraphicsContext();

		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Dynamic;
		vbDesc.elmSize = sizeof(QuadVertex);
		vbDesc.bufferSize = sizeof(QuadVertex) * MaxBatchCount * 4;
		g_quadVB = context.CreateVertexBuffer(vbDesc);

		VertexBuffer::Descriptor circleVBDesc = {};
		circleVBDesc.usage = UsageFlag::Dynamic;
		circleVBDesc.elmSize = sizeof(CircleVertex);
		circleVBDesc.bufferSize = sizeof(CircleVertex) * MaxBatchCount * 4;
		g_circleVB = context.CreateVertexBuffer(circleVBDesc);

		VertexBuffer::Descriptor lineVBDesc = {};
		lineVBDesc.usage = UsageFlag::Dynamic;
		lineVBDesc.elmSize = sizeof(LineVertex);
		lineVBDesc.bufferSize = sizeof(LineVertex) * MaxBatchCount * 2;
		g_lineVB = context.CreateVertexBuffer(lineVBDesc);

		VertexBuffer::Descriptor textVBDesc = {};
		textVBDesc.usage = UsageFlag::Dynamic;
		textVBDesc.elmSize = sizeof(QuadVertex);
		textVBDesc.bufferSize = sizeof(QuadVertex) * MaxBatchCount * 4;
		g_textVB = context.CreateVertexBuffer(textVBDesc);

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

		g_vpCB = context.CreateConstantBuffer(sizeof(CameraConstants));
		g_globalCB = context.CreateConstantBuffer(sizeof(GlobalConstants));

		Ref<GraphicsShader> shader = context.CreateGraphicsShader("Resources/Shaders/std2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<float>("COLOR", 4);
		shader->AddInputElement<uint32_t>("TEXTUREID", 1);
		shader->AddInputElement<uint32_t>("ID", 1);
		shader->CreateInputLayout();
		g_quadPipeline = context.CreateGraphicsPipeline();

		g_quadPipeline->SetShader(shader);
		g_quadPipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> circleShader = context.CreateGraphicsShader("Resources/Shaders/circle2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		circleShader->AddInputElement<float>("LOCALPOSITION", 3);
		circleShader->AddInputElement<float>("WORLDPOSITION", 3);
		circleShader->AddInputElement<float>("THICKNESS", 1);
		circleShader->AddInputElement<float>("COLOR", 4);
		circleShader->AddInputElement<uint32_t>("ID", 1);
		circleShader->CreateInputLayout();

		g_circlePipeline = context.CreateGraphicsPipeline();
		g_circlePipeline->SetShader(circleShader);
		g_circlePipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> lineShader = context.CreateGraphicsShader("Resources/Shaders/line2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		lineShader->AddInputElement<float>("POSITION", 3);
		lineShader->AddInputElement<float>("COLOR", 4);
		lineShader->AddInputElement<uint32_t>("ID", 1);
		lineShader->CreateInputLayout();

		g_linePipeline = context.CreateGraphicsPipeline();
		g_linePipeline->SetShader(lineShader);
		g_linePipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> textShader = context.CreateGraphicsShader("Resources/Shaders/text2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		textShader->AddInputElement<float>("POSITION", 3);
		textShader->AddInputElement<float>("TEXCOORD", 2);
		textShader->AddInputElement<float>("COLOR", 4);
		textShader->AddInputElement<uint32_t>("TEXTUREID", 1);
		textShader->AddInputElement<uint32_t>("ID", 1);
		textShader->CreateInputLayout();

		g_textPipeline = context.CreateGraphicsPipeline();
		g_textPipeline->SetShader(textShader);
		g_textPipeline->SetCullMode(CullMode::None);
	}

	void Renderer2D::Cleanup() {
		g_quadVB.reset();
		g_circleVB.reset();
		g_lineVB.reset();
		g_textVB.reset();
		g_ib.reset();
		g_vpCB.reset();
		g_quadPipeline.reset();
		g_circlePipeline.reset();
		g_linePipeline.reset();
		g_textPipeline.reset();
	}

	void Renderer2D::Begin(const mat4& view, const mat4& projection) {
		CameraConstants vp;
		vp.view = view;
		vp.projection = projection;
		g_vpCB->Update(&vp, sizeof(CameraConstants));

		GlobalConstants globalConstants;
		int32_t width, height;
		Graphics::GetSize(width, height);
		globalConstants.screenResolution = vec2((float)width, (float)height);
		globalConstants.time = Time::GetTime();
		globalConstants.deltaTime = Time::DeltaTime();
		g_globalCB->Update(&globalConstants, sizeof(GlobalConstants));

		g_quadVertices.clear();
		g_quadTextures.clear();
		g_quadIndexCount = 0;

		g_circleVertices.clear();
		g_circleIndexCount = 0;

		g_lineVertices.clear();
		g_lineIndexCount = 0;

		g_textVertices.clear();
		g_textTextures.clear();
		g_textIndexCount = 0;
	}

	void Renderer2D::End() {
		auto& mainPass = Graphics::GetMainRenderPass();
		auto& commandQueue = Graphics::GetCommandQueue();

		mainPass->SetBlendMode(0, BlendMode::Alpha, true);
		mainPass->Bind(false, false);

		g_quadVB->Update(g_quadVertices.data(), sizeof(QuadVertex), g_quadVertices.size());
		g_circleVB->Update(g_circleVertices.data(), sizeof(CircleVertex), g_circleVertices.size());
		g_lineVB->Update(g_lineVertices.data(), sizeof(LineVertex), g_lineVertices.size());
		g_textVB->Update(g_textVertices.data(), sizeof(QuadVertex), g_textVertices.size());

		commandQueue.SetConstantBuffer(g_vpCB, 0);
		commandQueue.SetConstantBuffer(g_globalCB, 1);

		if (g_quadIndexCount) {
			commandQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			commandQueue.SetPipeline(g_quadPipeline);
			commandQueue.SetVertexBuffer(g_quadVB);
			for (auto& [texture, id] : g_quadTextures) {
				commandQueue.SetTexture(texture, id);
			}
			commandQueue.DrawIndexed(g_ib, g_quadIndexCount);
		}

		if (g_circleIndexCount) {
			commandQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			commandQueue.SetPipeline(g_circlePipeline);
			commandQueue.SetVertexBuffer(g_circleVB);
			commandQueue.DrawIndexed(g_ib, g_circleIndexCount);
		}

		if (g_lineIndexCount) {
			commandQueue.SetPrimitiveTopology(PrimitiveTopology::LineList);
			commandQueue.SetPipeline(g_linePipeline);
			commandQueue.SetVertexBuffer(g_lineVB);
			commandQueue.Draw(g_lineIndexCount);
		}

		if (g_textIndexCount) {
			commandQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			commandQueue.SetPipeline(g_textPipeline);
			commandQueue.SetVertexBuffer(g_textVB);

			for (auto& [texture, id] : g_textTextures) {
				commandQueue.SetTexture(texture, id);
			}

			commandQueue.DrawIndexed(g_ib, g_textIndexCount);
		}
		
		commandQueue.Execute();
	}

	void Renderer2D::DrawQuad(const entt::entity id, const mat4& transform, const vec4& color) {
		uint32_t textureID = 0xFFFFFFFF;

		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f)), vec2(0.0f, 0.0f), color, textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, 0.5f, 0.0f, 1.0f)), vec2(1.0f, 0.0f), color, textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, -0.5f, 0.0f, 1.0f)), vec2(1.0f, 1.0f), color, textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f)), vec2(0.0f, 1.0f), color, textureID, (uint32_t)id });

		g_quadIndexCount += 6;
	}

	void Renderer2D::DrawQuad(const entt::entity id, const mat4& transform, const Ref<Texture2D>& texture) {
		uint32_t textureID;
		if (g_quadTextures.find(texture) == g_quadTextures.end()) {
			textureID = g_quadTextures.size();
			g_quadTextures[texture] = textureID;
		}
		else {
			textureID = g_quadTextures[texture];
		}

		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f)), vec2(0.0f, 0.0f), vec4(1.0f), textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, 0.5f, 0.0f, 1.0f)), vec2(1.0f, 0.0f), vec4(1.0f), textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(0.5f, -0.5f, 0.0f, 1.0f)), vec2(1.0f, 1.0f), vec4(1.0f), textureID, (uint32_t)id });
		g_quadVertices.push_back({ vec3(transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f)), vec2(0.0f, 1.0f), vec4(1.0f), textureID, (uint32_t)id });

		g_quadIndexCount += 6;
	}

	void Renderer2D::DrawCircle(const entt::entity id, const mat4& transform,  const vec4& color, const float thickness) {
		g_circleVertices.push_back({ vec3(-0.5f, 0.5f, 0.0f), vec3(transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f)), thickness, color, (uint32_t)id });
		g_circleVertices.push_back({ vec3(0.5f, 0.5f, 0.0f), vec3(transform * vec4(0.5f, 0.5f, 0.0f, 1.0f)), thickness, color, (uint32_t)id });
		g_circleVertices.push_back({ vec3(0.5f, -0.5f, 0.0f), vec3(transform * vec4(0.5f, -0.5f, 0.0f, 1.0f)), thickness, color, (uint32_t)id });
		g_circleVertices.push_back({ vec3(-0.5f, -0.5f, 0.0f), vec3(transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f)), thickness, color, (uint32_t)id });

		g_circleIndexCount += 6;
	}

	void Renderer2D::DrawLine(const entt::entity id, const vec3& start, const vec3& end, const vec4& color) {
		g_lineVertices.push_back({ start, color, (uint32_t)id });
		g_lineVertices.push_back({ end, color, (uint32_t)id });

		g_lineIndexCount += 2;
	}

	void Renderer2D::DrawLineRect(const entt::entity id, const mat4& transform, const vec4& color) {
		vec3 p0 = transform * vec4(-0.5f, 0.5f, 0.0f, 1.0f);
		vec3 p1 = transform * vec4(0.5f, 0.5f, 0.0f, 1.0f);
		vec3 p2 = transform * vec4(0.5f, -0.5f, 0.0f, 1.0f);
		vec3 p3 = transform * vec4(-0.5f, -0.5f, 0.0f, 1.0f);

		g_lineVertices.push_back({ p0, color, (uint32_t)id });
		g_lineVertices.push_back({ p1, color, (uint32_t)id });
		g_lineVertices.push_back({ p1, color, (uint32_t)id });
		g_lineVertices.push_back({ p2, color, (uint32_t)id });
		g_lineVertices.push_back({ p2, color, (uint32_t)id });
		g_lineVertices.push_back({ p3, color, (uint32_t)id });
		g_lineVertices.push_back({ p3, color, (uint32_t)id });
		g_lineVertices.push_back({ p0, color, (uint32_t)id });

		g_lineIndexCount += 8;
	}

	void Renderer2D::DrawString(const entt::entity id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const Ref<Texture2D>& fontAtlas, const vec4& color) {
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

			g_textVertices.push_back({ vec3(transform * vec4(quadMin.x, quadMax.y, 0.0f, 1.0f)), vec2(texcoordMin.x, texcoordMax.y), color, atlasID, (uint32_t)id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMax.x, quadMax.y, 0.0f, 1.0f)), vec2(texcoordMax.x, texcoordMax.y), color, atlasID, (uint32_t)id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMax.x, quadMin.y, 0.0f, 1.0f)), vec2(texcoordMax.x, texcoordMin.y), color, atlasID, (uint32_t)id });
			g_textVertices.push_back({ vec3(transform * vec4(quadMin.x, quadMin.y, 0.0f, 1.0f)), vec2(texcoordMin.x, texcoordMin.y), color, atlasID, (uint32_t)id });

			g_textIndexCount += 6;

			x += glyph.advance + kerningOffset;
		}
	}
}