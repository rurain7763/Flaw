#pragma once

#include "Core.h"
#include "Graphics/GraphicsCommandQueue.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;

	class DXCommandQueue : public GraphicsCommandQueue {
	public:
		DXCommandQueue(DXContext& context);
		virtual ~DXCommandQueue() = default;

		void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) override;
		void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;
		void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		void SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
		void SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) override;
		void SetTexture(const Ref<Texture2D>& texture, uint32_t slot) override;
		void SetTexture(const Ref<Texture2DArray>& texture, uint32_t slot) override;
		void SetTexture(const Ref<TextureCube>& texture, uint32_t slot) override;

		void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) override;
		void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
		void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;

		void SetComputePipeline(const Ref<ComputePipeline>& pipeline) override;
		void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
		void SetComputeTexture(const Ref<Texture2D>& texture, BindFlag bindFlag, uint32_t slot) override;
		void SetComputeTexture(const Ref<Texture2DArray>& texture, BindFlag bindFlag, uint32_t slot) override;
		void SetComputeTexture(const Ref<TextureCube>& texture, BindFlag bindFlag, uint32_t slot) override;
		void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) override;
		void Dispatch(uint32_t x, uint32_t y, uint32_t z) override;

		void Execute() override;

	private:
		void ClearAllRegistries();

		void BindToGraphicsTRegistry(uint32_t slot, const ComPtr<ID3D11ShaderResourceView>& srv);

		void BindToComputeTRegistry(uint32_t slot, const ComPtr<ID3D11ShaderResourceView>& srv);
		void BindToComputeURegistry(uint32_t slot, const ComPtr<ID3D11UnorderedAccessView>& uav);

	private:
		DXContext& _context;

		std::queue<std::function<void()>> _commands;
	};
}