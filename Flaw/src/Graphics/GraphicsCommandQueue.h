#pragma once

#include "Core.h"
#include "Graphics/GraphicsType.h"
#include "Graphics/GraphicsBuffers.h"
#include "Graphics/GraphicsPipeline.h"
#include "Graphics/Texture.h"

namespace flaw {
	class GraphicsCommandQueue {
	public:
		virtual void SetPipeline(const Ref<GraphicsPipeline>& pipeline) = 0;
		virtual void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) = 0;
		virtual void SetTexture(const Ref<Texture2D>& texture, uint32_t slot) = 0;
		virtual void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) = 0;

		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Execute() = 0;
	};
}