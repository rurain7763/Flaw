#include "pch.h"
#include "Renderer2D.h"

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

		Ref<GraphicsShader> shader = _context.CreateGraphicsShader("Contents/Shaders/std2d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<uint32_t>("ID", 1);
		shader->CreateInputLayout();

		_pipeline = _context.CreateGraphicsPipeline();
		_pipeline->SetShader(shader);
		_pipeline->SetBlendMode(BlendMode::Alpha, true);
		_pipeline->SetCullMode(CullMode::None);
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
}