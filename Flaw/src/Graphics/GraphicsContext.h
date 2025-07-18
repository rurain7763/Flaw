#pragma once

#include "Core.h"
#include "Platform/PlatformContext.h"
#include "GraphicsBuffers.h"
#include "GraphicsShader.h"
#include "GraphicsPipeline.h"
#include "ComputeShader.h"
#include "ComputePipeline.h"
#include "GraphicsCommandQueue.h"
#include "GraphicsRenderPass.h"
#include "Texture.h"

namespace flaw {
	class FAPI GraphicsContext {
	public:
		GraphicsContext() = default;
		virtual ~GraphicsContext() = default;

		virtual void Prepare() = 0;
		virtual void Present() = 0;

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) = 0;
		virtual Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) = 0;
		virtual Ref<GraphicsPipeline> CreateGraphicsPipeline() = 0;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size) = 0;
		virtual Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) = 0;

		virtual Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) = 0;
		virtual Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor) = 0;
		virtual Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) = 0;

		virtual Ref<GraphicsRenderPass> GetMainRenderPass() = 0;

		virtual GraphicsCommandQueue& GetCommandQueue() = 0;

		virtual Ref<GraphicsRenderPass> CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) = 0;
		virtual void SetRenderPass(GraphicsRenderPass* renderPass) = 0;
		virtual void ResetRenderPass() = 0;

		virtual void Resize(int32_t width, int32_t height) = 0;
		virtual void GetSize(int32_t& width, int32_t& height) = 0;

		virtual Ref<ComputeShader> CreateComputeShader(const char* filename) = 0;
		virtual Ref<ComputePipeline> CreateComputePipeline() = 0;
	};
}