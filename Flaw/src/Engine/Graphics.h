#pragma once

#include "Core.h"
#include "Graphics/GraphicsContext.h"

namespace flaw {
	enum class GraphicsType {
		DX11
	};

	class Graphics {
	public:
		static void Init(GraphicsType type);
		static void Cleanup();

		static void Prepare();
		static void Present();

		static Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor);
		static Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor);
		static Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag);
		static Ref<GraphicsPipeline> CreateGraphicsPipeline();

		static Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size);
		static Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc);

		static Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor);

		static void SetRenderTexture(uint32_t slot, Ref<Texture2D> texture, float clearValue[4]);
		static void ResetRenderTexture(uint32_t slot);

		static GraphicsCommandQueue& GetCommandQueue();

		static void CaptureRenderTargetTex(Ref<Texture2D>& dstTexture);

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height);

		static void SetClearColor(float r, float g, float b, float a);

		static void Resize(int32_t width, int32_t height);

		static GraphicsContext& GetGraphicsContext();
	};
}