#include "pch.h"
#include "Renderer2D.h"
#include "Font/FontData.h"

namespace flaw {
	Renderer2D::Renderer2D(GraphicsContext& context)
		: _context(context)
		, _commandQueue(*context.GetCommandQueue().get())
	{
		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Dynamic;
		vbDesc.elmSize = sizeof(Vertex);
		vbDesc.count = 4;

		_vb = _context.CreateVertexBuffer(vbDesc);

		_ib = _context.CreateIndexBuffer();

		const uint32_t indices[] = {
			0, 1, 2, 0, 2, 3
		};

		_ib->Update(indices, 6);

		_mvpCBuff = _context.CreateConstantBuffer(sizeof(MVPMatrices));
		_constCBuff = _context.CreateConstantBuffer(sizeof(ConstDatas));

		Ref<GraphicsShader> shader = _context.CreateGraphicsShader("Resources/Shaders/std2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<uint32_t>("ID", 1);
		shader->CreateInputLayout();

		_pipeline = _context.CreateGraphicsPipeline();
		_pipeline->SetShader(shader);
		_pipeline->SetBlendMode(BlendMode::Alpha, true);
		_pipeline->SetCullMode(CullMode::None);

		Ref<GraphicsShader> textShader = _context.CreateGraphicsShader("Resources/Shaders/text2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		textShader->AddInputElement<float>("POSITION", 3);
		textShader->AddInputElement<float>("TEXCOORD", 2);
		textShader->AddInputElement<uint32_t>("ID", 1);
		textShader->CreateInputLayout();

		_textPipeline = _context.CreateGraphicsPipeline();
		_textPipeline->SetShader(textShader);
		_textPipeline->SetBlendMode(BlendMode::Alpha, true);
		_textPipeline->SetCullMode(CullMode::None);
	}

	void Renderer2D::Begin(const mat4& view, const mat4& projection) {
		_mvp.view = view;
		_mvp.projection = projection;
	}

	void Renderer2D::End() {
		// TODO: 최적화를 위해 쿼드를 여기서 그려야 함
		// 현 상태는 드로우 호출 시 마다 쿼드를 그림
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const vec4& color) {
		for (uint32_t i = 0; i < 4; i++) {
			_defaultQuadVertices[i].id = id;
		}

		_vb->Update(_defaultQuadVertices, sizeof(Vertex), 4);
		
		_mvp.model = transform;

		_constDatas.textureEnabled = 0;
		_constDatas.color = color;

		_mvpCBuff->Update(&_mvp, sizeof(MVPMatrices));
		_constCBuff->Update(&_constDatas, sizeof(ConstDatas));

		_commandQueue.Begin();
		_commandQueue.SetPipeline(_pipeline);
		_commandQueue.SetVertexBuffer(_vb);
		_commandQueue.SetConstantBuffer(_mvpCBuff, 0);
		_commandQueue.SetConstantBuffer(_constCBuff, 1);
		_commandQueue.DrawIndexed(_ib);
		_commandQueue.End();

		_commandQueue.Execute();
	}

	void Renderer2D::DrawQuad(const uint32_t id, const mat4& transform, const Ref<Texture>& texture) {
		for (uint32_t i = 0; i < 4; i++) {
			_defaultQuadVertices[i].id = id;
		}

		_vb->Update(_defaultQuadVertices, sizeof(Vertex), 4);
		
		_mvp.model = transform;

		_constDatas.textureEnabled = 1;
		_constDatas.color = vec4(1.0f);

		_mvpCBuff->Update(&_mvp, sizeof(MVPMatrices));
		_constCBuff->Update(&_constDatas, sizeof(ConstDatas));

		_commandQueue.Begin();
		_commandQueue.SetPipeline(_pipeline);
		_commandQueue.SetVertexBuffer(_vb);
		_commandQueue.SetConstantBuffer(_mvpCBuff, 0);
		_commandQueue.SetConstantBuffer(_constCBuff, 1);
		_commandQueue.SetTexture(texture, 0);
		_commandQueue.DrawIndexed(_ib);
		_commandQueue.End();

		_commandQueue.Execute();
	}

	static Ref<Texture> g_texture;

	void Renderer2D::DrawString(const uint32_t id, const mat4& transform, const std::wstring text, const Ref<Font>& font, const vec4& color) {
		_mvp.model = transform;

		_constDatas.textureEnabled = 1;
		_constDatas.color = color;

		_mvpCBuff->Update(&_mvp, sizeof(MVPMatrices));
		_constCBuff->Update(&_constDatas, sizeof(ConstDatas));
		
		auto& fontData = font->GetFontData();
		const auto& metrics = fontData.geometry.getMetrics();

		// TODO: test
		if (!g_texture) {
			Texture::Descriptor desc = {};
			desc.type = Texture::Type::Texture2D;
			desc.width = fontData.atlas.width;
			desc.height = fontData.atlas.height;
			desc.format = PixelFormat::RGBA8;
			desc.data = fontData.atlas.data.data();
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::ShaderResource;

			g_texture = _context.CreateTexture2D(desc);
		}

		float x = 0.0f;
		float fsScale = 1.0f / (metrics.ascenderY - metrics.descenderY);
		float y = 0.0f;
		float lineOffset = 0.0f;
		float kerningOffset = 0.0f;

		for (int32_t i = 0; i < text.size(); ++i) {
			uint32_t uniChar = text[i];

			if (uniChar == L'\r') {
				continue;
			}

			if (uniChar == L'\n') {
				x = 0.0f;
				y -= metrics.lineHeight * fsScale - lineOffset;
				continue;
			}

			if (uniChar == L'\t') {
				auto blankGlyph = fontData.geometry.getGlyph(' ');
				if (blankGlyph) {
					double advance = blankGlyph->getAdvance();
					fontData.geometry.getAdvance(advance, ' ', 0);
					x += (advance * fsScale + kerningOffset) * 4;
					continue;
				}
			}

			if (uniChar == L' ') {
				auto blankGlyph = fontData.geometry.getGlyph(' ');
				if (blankGlyph) {
					double advance = blankGlyph->getAdvance();
					fontData.geometry.getAdvance(advance, ' ', i == text.size() - 1 ? 0 : text[i + 1]);
					x += advance * fsScale + kerningOffset;
					continue;
				}
			}

			auto glyph = fontData.geometry.getGlyph(uniChar);

			if (!glyph) {
				glyph = fontData.geometry.getGlyph('?');

				if (!glyph) {
					continue;
				}
			}

			double l, b, r, t;
			glyph->getQuadAtlasBounds(l, b, r, t);
			vec2 texcoordMin = vec2(l, b);
			vec2 texcoordMax = vec2(r, t);

			glyph->getQuadPlaneBounds(l, b, r, t);
			vec2 quadMin = vec2(l, b);
			vec2 quadMax = vec2(r, t);

			quadMin *= fsScale; quadMax *= fsScale;
			quadMin += vec2(x, y); quadMax += vec2(x, y);

			float texelW = 1.0f / fontData.atlas.width;
			float texelH = 1.0f / fontData.atlas.height;

			texcoordMin *= vec2(texelW, texelH);
			texcoordMax *= vec2(texelW, texelH);

			// rendering
			Vertex vertices[4] = {
				{ vec3(quadMin.x, quadMax.y, 0.0f), vec2(texcoordMin.x, texcoordMax.y), id },
				{ vec3(quadMax.x, quadMax.y, 0.0f), vec2(texcoordMax.x, texcoordMax.y), id },
				{ vec3(quadMax.x, quadMin.y, 0.0f), vec2(texcoordMax.x, texcoordMin.y), id },
				{ vec3(quadMin.x, quadMin.y, 0.0f), vec2(texcoordMin.x, texcoordMin.y), id }
			};

			_vb->Update(vertices, sizeof(Vertex), 4);

			_commandQueue.Begin();
			_commandQueue.SetPipeline(_textPipeline);
			_commandQueue.SetVertexBuffer(_vb);
			_commandQueue.SetConstantBuffer(_mvpCBuff, 0);
			_commandQueue.SetConstantBuffer(_constCBuff, 1);
			_commandQueue.SetTexture(g_texture, 0);
			_commandQueue.DrawIndexed(_ib);
			_commandQueue.End();

			_commandQueue.Execute();

			double advance = glyph->getAdvance();
			fontData.geometry.getAdvance(advance, text[i], i == text.size() - 1 ? 0 : text[i + 1]);

			x += advance * fsScale + kerningOffset;
		}
	}
}