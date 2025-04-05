#include "pch.h"
#include "DXCommandQueue.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
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