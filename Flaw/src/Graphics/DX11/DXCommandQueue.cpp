#include "pch.h"
#include "DXCommandQueue.h"
#include "DXTextures.h"
#include "DXContext.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	static std::unordered_map<uint32_t, Ref<Texture>> g_bindedTextures;
	static std::unordered_map<uint32_t, Ref<StructuredBuffer>> g_bindedStructuredBuffers;

	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
	}

	void DXCommandQueue::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		_commands.push([this, primitiveTopology]() { 
			_context.DeviceContext()->IASetPrimitiveTopology(ConvertToD3D11Topology(primitiveTopology));
		});
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		_commands.push([this, pipeline]() { 
			ResetAllTextures();
			pipeline->Bind(); 
		});
	}

	void DXCommandQueue::SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) {
		_commands.push([vertexBuffer]() { vertexBuffer->Bind(); });
	}

	void DXCommandQueue::SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		_commands.push([constantBuffer, slot]() { 
			constantBuffer->Unbind();
			constantBuffer->BindToGraphicsShader(slot); 
		});
	}

	void DXCommandQueue::SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) {
		_commands.push([this, buffer, slot]() {
			ResetTexture(slot);
			buffer->BindToGraphicsShader(slot);
			g_bindedStructuredBuffers[slot] = buffer;
		});
	}

	void DXCommandQueue::SetTexture(const Ref<Texture>& texture, uint32_t slot) {
		_commands.push([this, texture, slot]() { 
			ResetTexture(slot);
			texture->BindToGraphicsShader(slot);
			g_bindedTextures[slot] = texture;
		});
	}

	void DXCommandQueue::Draw(uint32_t vertexCount, uint32_t vertexOffset) {
		_commands.push([this, vertexCount, vertexOffset]() { _context.DeviceContext()->Draw(vertexCount, vertexOffset); });
	}

	void DXCommandQueue::DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset, uint32_t vertexOffset) {
		_commands.push([this, indexBuffer, indexCount, indexOffset, vertexOffset]() {
			indexBuffer->Bind();
			_context.DeviceContext()->DrawIndexed(indexCount, indexOffset, vertexOffset);
		});
	}

	void DXCommandQueue::DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset, uint32_t vertexOffset) {
		_commands.push([this, indexBuffer, indexCount, instanceCount, indexOffset, vertexOffset]() {
			indexBuffer->Bind();
			_context.DeviceContext()->DrawIndexedInstanced(indexCount, instanceCount, indexOffset, vertexOffset, 0);
		});
	}

	void DXCommandQueue::SetComputePipeline(const Ref<ComputePipeline>& pipeline) {
		_commands.push([pipeline]() { pipeline->Bind(); });
	}

	void DXCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		_commands.push([constantBuffer, slot]() { 
			constantBuffer->Unbind();
			constantBuffer->BindToComputeShader(slot); 
		});
	}

	void DXCommandQueue::SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, texture, bindFlag, slot]() { 
			ResetTexture(slot);
			texture->BindToComputeShader(bindFlag, slot); 
			g_bindedTextures[slot] = texture;
		});
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, buffer, bindFlag, slot]() { 
			ResetTexture(slot);
			buffer->BindToComputeShader(bindFlag, slot); 
			g_bindedStructuredBuffers[slot] = buffer;
		});
	}

	void DXCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		_commands.push([this, x, y, z]() { _context.DeviceContext()->Dispatch(x, y, z); });
	}

	void DXCommandQueue::ResetTexture(const uint32_t slot) {
		auto bindedTextureIt = g_bindedTextures.find(slot);
		if (bindedTextureIt != g_bindedTextures.end()) {
			bindedTextureIt->second->Unbind();
			g_bindedTextures.erase(bindedTextureIt);
		}

		auto bindedStructuredBufferIt = g_bindedStructuredBuffers.find(slot);
		if (bindedStructuredBufferIt != g_bindedStructuredBuffers.end()) {
			bindedStructuredBufferIt->second->Unbind();
			g_bindedStructuredBuffers.erase(bindedStructuredBufferIt);
		}
	}

	void DXCommandQueue::ResetAllTextures() {
		for (auto& [slot, texture] : g_bindedTextures) {
			texture->Unbind();
		}
		g_bindedTextures.clear();

		for (auto& [slot, buffer] : g_bindedStructuredBuffers) {
			buffer->Unbind();
		}
		g_bindedStructuredBuffers.clear();
	}

	void DXCommandQueue::Execute() {
		while (!_commands.empty()) {
			_commands.front()();
			_commands.pop();
		}
	}
}