#include "pch.h"
#include "DXCommandQueue.h"
#include "DXTextures.h"
#include "DXContext.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
	}

	void DXCommandQueue::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		FASSERT(_open, "DXCommandQueue::SetPrimitiveTopology failed: Command queue is not open");
		_commands.push([this, primitiveTopology]() { 
			_context.DeviceContext()->IASetPrimitiveTopology(ConvertToD3D11Topology(primitiveTopology));
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
		_commands.push([constantBuffer, slot]() { 
			constantBuffer->Unbind();
			constantBuffer->BindToGraphicsShader(slot); 
		});
	}

	void DXCommandQueue::SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetStructuredBuffer failed: Command queue is not open");
		_commands.push([buffer, slot]() {
			buffer->Unbind();
			buffer->BindToGraphicsShader(slot);
		});
	}

	void DXCommandQueue::SetTexture(const Ref<Texture>& texture, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetTexture failed: Command queue is not open");
		_commands.push([texture, slot]() { 
			texture->Unbind();
			texture->BindToGraphicsShader(slot);
		});
	}

	void DXCommandQueue::SetTextures(const Ref<Texture>* textures, uint32_t count, uint32_t startSlot) {
		FASSERT(_open, "DXCommandQueue::SetTextures failed: Command queue is not open");
		_commands.push([this, textures, count, startSlot]() {
			for (uint32_t i = 0; i < count; ++i) {
				textures[i]->Unbind();
				textures[i]->BindToGraphicsShader(startSlot + i);
			}
		});
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

	void DXCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
		FASSERT(_open, "DXCommandQueue::SetComputePipeline failed: Command queue is not open");
		_commands.push([pipeline]() { pipeline->Bind(); });
	}

	void DXCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetComputeConstantBuffer failed: Command queue is not open");
		_commands.push([constantBuffer, slot]() { 
			constantBuffer->Unbind();
			constantBuffer->BindToComputeShader(slot); 
		});
	}

	void DXCommandQueue::SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetComputeTexture failed: Command queue is not open");
		_commands.push([texture, bindFlag, slot]() { 
			texture->Unbind();
			texture->BindToComputeShader(bindFlag, slot); 
		});
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) {
		FASSERT(_open, "DXCommandQueue::SetComputeStructuredBuffer failed: Command queue is not open");
		_commands.push([buffer, bindFlag, slot]() { 
			buffer->Unbind();
			buffer->BindToComputeShader(bindFlag, slot); 
		});
	}

	void DXCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		FASSERT(_open, "DXCommandQueue::Dispatch failed: Command queue is not open");
		_commands.push([this, x, y, z]() { _context.DeviceContext()->Dispatch(x, y, z); });
	}

	void DXCommandQueue::ResetAllTextures() {
		FASSERT(_open, "DXCommandQueue::ResetAllTextures failed: Command queue is not open");
		_commands.push([this]() {
			ID3D11ShaderResourceView* nullSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = { nullptr };
			_context.DeviceContext()->PSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
			_context.DeviceContext()->VSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
			_context.DeviceContext()->GSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
			_context.DeviceContext()->HSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
			_context.DeviceContext()->DSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
			_context.DeviceContext()->CSSetShaderResources(0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullSRVs);
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