#include "pch.h"
#include "DXCommandQueue.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
	}

	void DXCommandQueue::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		FASSERT(_open, "DXCommandQueue::SetPrimitiveTopology failed: Command queue is not open");
		_commands.push([this, primitiveTopology]() { 
			switch (primitiveTopology) {
				case PrimitiveTopology::PointList: _context.DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST); break;
				case PrimitiveTopology::LineList: _context.DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST); break;
				case PrimitiveTopology::LineStrip: _context.DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP); break;
				case PrimitiveTopology::TriangleList: _context.DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); break;
				case PrimitiveTopology::TriangleStrip: _context.DeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); break;
			}
		});
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		FASSERT(_open, "DXCommandQueue::SetPipeline failed: Command queue is not open");
		_commands.push([pipeline]() { pipeline->Bind(); });
	}

	void DXCommandQueue::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
		FASSERT(_open, "DXCommandQueue::SetVertexBuffer failed: Command queue is not open");
		_commands.push([vertexBuffer]() { vertexBuffer->Bind(); });
	}

	void DXCommandQueue::SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetConstantBuffer failed: Command queue is not open");
		_commands.push([constantBuffer, slot]() { constantBuffer->BindToGraphicsShader(slot); });
	}

	void DXCommandQueue::SetTexture(const Ref<Texture2D>& texture, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetTexture failed: Command queue is not open");
		_commands.push([texture, slot]() { texture->BindToGraphicsShader(slot); });
	}

	void DXCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
		FASSERT(_open, "DXCommandQueue::Draw failed: Command queue is not open");
		_commands.push([this, vertexCount, vertexOffset]() { _context.DeviceContext()->Draw(vertexCount, vertexOffset); });
	}

	void DXCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
		FASSERT(_open, "DXCommandQueue::DrawIndexed failed: Command queue is not open");
		_commands.push([this, indexBuffer, indexCount, indexOffset, vertexOffset]() {
			indexBuffer->Bind();
			_context.DeviceContext()->DrawIndexed(indexCount, indexOffset, vertexOffset);
		});
	}

	void DXCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
		FASSERT(_open, "DXCommandQueue::DrawIndexedInstanced failed: Command queue is not open");
		_commands.push([this, indexBuffer, indexCount, instanceCount, indexOffset, vertexOffset]() {
			indexBuffer->Bind();
			_context.DeviceContext()->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, 0);
		});
	}

	void DXCommandQueue::Begin() {
		_open = true;
	}

	void DXCommandQueue::End() {
		_open = false;
	}

	void DXCommandQueue::Execute() {
		while (!_commands.empty()) {
			_commands.front()();
			_commands.pop();
		}
	}
}