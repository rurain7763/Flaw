#include "pch.h"
#include "DXCommandQueue.h"
#include "DXTextures.h"
#include "DXStructuredBuffer.h"
#include "DXContext.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	static std::unordered_map<uint32_t, ComPtr<ID3D11ShaderResourceView>> g_graphicsTRegistries;

	static std::unordered_map<uint32_t, ComPtr<ID3D11ShaderResourceView>> g_computeTRegistries;
	static std::unordered_map<uint32_t, ComPtr<ID3D11UnorderedAccessView>> g_computeURegistries;

	DXCommandQueue::DXCommandQueue(DXContext& context)
		: _context(context)
	{
	}

	void DXCommandQueue::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		_commands.push([this, primitiveTopology]() { 
			_context.DeviceContext()->IASetPrimitiveTopology(ConvertToD3D11Topology(primitiveTopology));
		});
	}

	void DXCommandQueue::ClearAllRegistries() {
		g_graphicsTRegistries.clear();
		g_computeTRegistries.clear();
		g_computeURegistries.clear();

		std::vector<ID3D11ShaderResourceView*> nullSRVs(D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, nullptr);
		_context.DeviceContext()->VSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->PSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->GSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->HSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->DSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());
		_context.DeviceContext()->CSSetShaderResources(0, nullSRVs.size(), nullSRVs.data());

		std::vector<ID3D11UnorderedAccessView*> nullUAVs(D3D11_PS_CS_UAV_REGISTER_COUNT, nullptr);
		_context.DeviceContext()->CSSetUnorderedAccessViews(0, nullUAVs.size(), nullUAVs.data(), nullptr);
	}

	void DXCommandQueue::SetPipeline(const Ref<GraphicsPipeline>& pipeline) {
		_commands.push([this, pipeline]() { 
			ClearAllRegistries(); // Clear all registries before binding a new pipeline
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

	void DXCommandQueue::BindToGraphicsTRegistry(uint32_t slot, const ComPtr<ID3D11ShaderResourceView>& srv) {
		auto it = g_graphicsTRegistries.find(slot);
		if (it != g_graphicsTRegistries.end() && it->second == srv) {
			return; // Already bound
		}

		_context.DeviceContext()->VSSetShaderResources(slot, 1, srv.GetAddressOf());
		_context.DeviceContext()->PSSetShaderResources(slot, 1, srv.GetAddressOf());
		_context.DeviceContext()->GSSetShaderResources(slot, 1, srv.GetAddressOf());
		_context.DeviceContext()->HSSetShaderResources(slot, 1, srv.GetAddressOf());
		_context.DeviceContext()->DSSetShaderResources(slot, 1, srv.GetAddressOf());

		g_graphicsTRegistries[slot] = srv;
	}

	void DXCommandQueue::SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) {
		_commands.push([this, buffer, slot]() {
			auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
			auto srv = dxSBuffer->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
				return;
			}

			BindToGraphicsTRegistry(slot, srv);
		});
	}

	void DXCommandQueue::SetTexture(const Ref<Texture2D>& texture, uint32_t slot) {
		_commands.push([this, texture, slot]() { 
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
				return;
			}

			BindToGraphicsTRegistry(slot, srv);
		});
	}

	void DXCommandQueue::SetTexture(const Ref<Texture2DArray>& texture, uint32_t slot) {
		_commands.push([this, texture, slot]() {
			auto dxTexture = std::static_pointer_cast<DXTexture2DArray>(texture);
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture array at slot %d", slot);
				return;
			}

			BindToGraphicsTRegistry(slot, srv);
		});
	}

	void DXCommandQueue::SetTexture(const Ref<TextureCube>& texture, uint32_t slot) {
		_commands.push([this, texture, slot]() {
			auto dxTexture = std::static_pointer_cast<DXTextureCube>(texture);
			auto srv = dxTexture->GetShaderResourceView();
			if (!srv) {
				Log::Error("ShaderResourceView is nullptr for texture cube at slot %d", slot);
				return;
			}

			BindToGraphicsTRegistry(slot, srv);
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
		_commands.push([this, pipeline]() { 
			ClearAllRegistries(); // Clear all registries before binding a new compute pipeline
			pipeline->Bind(); 
		});
	}

	void DXCommandQueue::SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) {
		_commands.push([constantBuffer, slot]() { 
			constantBuffer->Unbind();
			constantBuffer->BindToComputeShader(slot); 
		});
	}

	void DXCommandQueue::BindToComputeTRegistry(uint32_t slot, const ComPtr<ID3D11ShaderResourceView>& srv) {
		auto it = g_computeTRegistries.find(slot);
		if (it != g_computeTRegistries.end() && it->second == srv) {
			return;
		}

		_context.DeviceContext()->CSSetShaderResources(slot, 1, srv.GetAddressOf());

		g_computeTRegistries[slot] = srv;
	}

	void DXCommandQueue::BindToComputeURegistry(uint32_t slot, const ComPtr<ID3D11UnorderedAccessView>& uav) {
		auto it = g_computeURegistries.find(slot);
		if (it != g_computeURegistries.end() && it->second == uav) {
			return;
		}

		_context.DeviceContext()->CSSetUnorderedAccessViews(slot, 1, uav.GetAddressOf(), nullptr);

		g_computeURegistries[slot] = uav;
	}

	void DXCommandQueue::SetComputeTexture(const Ref<Texture2D>& texture, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, texture, bindFlag, slot]() { 
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);

			if (bindFlag & BindFlag::ShaderResource) {
				auto srv = dxTexture->GetShaderResourceView();
				if (!srv) {
					Log::Error("ShaderResourceView is nullptr for texture at slot %d", slot);
					return;
				}

				BindToComputeTRegistry(slot, srv);
			}
			else if (bindFlag & BindFlag::UnorderedAccess) {
				auto uav = dxTexture->GetUnorderedAccessView();
				if (!uav) {
					Log::Error("UnorderedAccessView is nullptr for texture at slot %d", slot);
					return;
				}

				BindToComputeURegistry(slot, uav);
			}
		});
	}

	void DXCommandQueue::SetComputeTexture(const Ref<Texture2DArray>& texture, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, texture, bindFlag, slot]() {
			auto dxTexture = std::static_pointer_cast<DXTexture2DArray>(texture);
			if (bindFlag & BindFlag::ShaderResource) {
				auto srv = dxTexture->GetShaderResourceView();
				if (!srv) {
					Log::Error("ShaderResourceView is nullptr for texture array at slot %d", slot);
					return;
				}

				BindToComputeTRegistry(slot, srv);
			}
			else if (bindFlag & BindFlag::UnorderedAccess) {
				auto uav = dxTexture->GetUnorderedAccessView();
				if (!uav) {
					Log::Error("UnorderedAccessView is nullptr for texture array at slot %d", slot);
					return;
				}

				BindToComputeURegistry(slot, uav);
			}
		});
	}

	void DXCommandQueue::SetComputeTexture(const Ref<TextureCube>& texture, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, texture, bindFlag, slot]() {
			auto dxTexture = std::static_pointer_cast<DXTextureCube>(texture);
			if (bindFlag & BindFlag::ShaderResource) {
				auto srv = dxTexture->GetShaderResourceView();
				if (!srv) {
					Log::Error("ShaderResourceView is nullptr for texture cube at slot %d", slot);
					return;
				}

				BindToComputeTRegistry(slot, srv);
			}
			else if (bindFlag & BindFlag::UnorderedAccess) {
				Log::Error("UnorderedAccessView is not supported for texture cube");
			}
		});
	}

	void DXCommandQueue::SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) {
		_commands.push([this, buffer, bindFlag, slot]() { 
			auto dxSBuffer = std::static_pointer_cast<DXStructuredBuffer>(buffer);
			if (bindFlag & BindFlag::ShaderResource) {
				auto srv = dxSBuffer->GetShaderResourceView();
				if (!srv) {
					Log::Error("ShaderResourceView is nullptr for structured buffer at slot %d", slot);
					return;
				}

				BindToComputeTRegistry(slot, srv);
			}
			else if (bindFlag & BindFlag::UnorderedAccess) {
				auto uav = dxSBuffer->GetUnorderedAccessView();
				if (!uav) {
					Log::Error("UnorderedAccessView is nullptr for structured buffer at slot %d", slot);
					return;
				}

				BindToComputeURegistry(slot, uav);
			}
		});
	}

	void DXCommandQueue::Dispatch(uint32_t x, uint32_t y, uint32_t z) {
		_commands.push([this, x, y, z]() { _context.DeviceContext()->Dispatch(x, y, z); });
	}

	void DXCommandQueue::Execute() {
		while (!_commands.empty()) {
			_commands.front()();
			_commands.pop();
		}
	}
}