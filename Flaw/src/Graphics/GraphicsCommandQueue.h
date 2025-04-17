#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"
#include "Graphics/GraphicsBuffers.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/ComputePipeline.h"
#include "Graphics/Texture.h"

namespace flaw {
	class GraphicsCommandQueue {
	public:
		virtual void SetPrimitiveTopology(PrimitiveTopology primitiveTopology) = 0;
		virtual void SetPipeline(const Ref<GraphicsPipeline>& pipeline) = 0;
		virtual void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) = 0;
		virtual void SetStructuredBuffer(const Ref<StructuredBuffer>& buffer, uint32_t slot) = 0;
		virtual void SetTexture(const Ref<Texture>& texture, uint32_t slot) = 0;
		virtual void SetTextures(const Ref<Texture>* textures, uint32_t count, uint32_t startSlot) = 0;
		
		virtual void Draw(uint32_t vertexCount, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
		virtual void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;
		
		virtual void SetComputePipeline(const Ref<ComputePipeline>& pipeline) = 0;
		virtual void SetComputeConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) = 0;
		virtual void SetComputeTexture(const Ref<Texture>& texture, BindFlag bindFlag, uint32_t slot) = 0;
		virtual void SetComputeStructuredBuffer(const Ref<StructuredBuffer>& buffer, BindFlag bindFlag, uint32_t slot) = 0;
		virtual void Dispatch(uint32_t x, uint32_t y, uint32_t z) = 0;

		virtual void ResetAllTextures() = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Execute() = 0;
	};
}