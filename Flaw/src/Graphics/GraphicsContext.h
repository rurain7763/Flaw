#pragma once

#include "Core.h"
#include "Platform/PlatformContext.h"
#include "GraphicsBuffers.h"
#include "GraphicsShader.h"
#include "GraphicsPipeline.h"
#include "GraphicsCommandQueue.h"
#include "Texture.h"

namespace flaw {
	class FAPI GraphicsContext {
	public:
		GraphicsContext() = default;
		virtual ~GraphicsContext() = default;

		virtual void Prepare() = 0;
		virtual void Present() = 0;

		virtual Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer() = 0;
		virtual Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) = 0;
		virtual Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) = 0;
		virtual Ref<GraphicsPipeline> CreateGraphicsPipeline() = 0;

		virtual Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size) = 0;
		virtual Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) = 0;

		virtual Ref<Texture> CreateTexture2D(const Texture::Descriptor& descriptor) = 0;

		virtual void SetRenderTexture(uint32_t slot, Ref<Texture> texture, float clearValue[4]) = 0;
		virtual void ResetRenderTexture(uint32_t slot) = 0;

		virtual Ref<GraphicsCommandQueue> GetCommandQueue() = 0;

		virtual void CaptureRenderTargetTex(Ref<Texture>& dstTexture) = 0;
		
		virtual void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
		virtual void GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height) = 0;

		virtual void SetClearColor(float r, float g, float b, float a) = 0;

		virtual void Resize(int32_t width, int32_t height) = 0;
	};
}