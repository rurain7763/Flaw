#include "pch.h"
#include "Graphics.h"
#include "Platform.h"
#include "Graphics/DX11/DXContext.h"

namespace flaw {
	static Scope<GraphicsContext> g_graphicsContext;

	void Graphics::Init(GraphicsType type) {
		int32_t width, height;
		Platform::GetFrameBufferSize(width, height);

		switch (type)
		{
		case flaw::GraphicsType::DX11:
			g_graphicsContext = CreateScope<DXContext>(Platform::GetPlatformContext(), width, height);
			break;
		default:
			throw std::runtime_error("Unsupported graphics type");
			break;
		}
	}

	void Graphics::Cleanup() {
		g_graphicsContext.reset();
	}

	void Graphics::Prepare() {
		g_graphicsContext->Prepare();
	}

	void Graphics::Present() {
		g_graphicsContext->Present();
	}

	Ref<VertexBuffer> Graphics::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
		return g_graphicsContext->CreateVertexBuffer(descriptor);
	}

	Ref<IndexBuffer> Graphics::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
		return g_graphicsContext->CreateIndexBuffer(descriptor);
	}

	Ref<GraphicsShader> Graphics::CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) {
		return g_graphicsContext->CreateGraphicsShader(filePath, compileFlag);
	}

	Ref<GraphicsPipeline> Graphics::CreateGraphicsPipeline() {
		return g_graphicsContext->CreateGraphicsPipeline();
	}

	Ref<ConstantBuffer> Graphics::CreateConstantBuffer(uint32_t size) {
		return g_graphicsContext->CreateConstantBuffer(size);
	}

	Ref<StructuredBuffer> Graphics::CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) {
		return g_graphicsContext->CreateStructuredBuffer(desc);
	}

	Ref<Texture2D> Graphics::CreateTexture2D(const Texture2D::Descriptor& descriptor) {
		return g_graphicsContext->CreateTexture2D(descriptor);
	}

	void Graphics::SetRenderTexture(uint32_t slot, Ref<Texture2D> texture, float clearValue[4]) {
		g_graphicsContext->SetRenderTexture(slot, texture, clearValue);
	}

	void Graphics::ResetRenderTexture(uint32_t slot) {
		g_graphicsContext->ResetRenderTexture(slot);
	}

	GraphicsCommandQueue& Graphics::GetCommandQueue() {
		return g_graphicsContext->GetCommandQueue();
	}

	void Graphics::CaptureRenderTargetTex(Ref<Texture2D>& dstTexture) {
		g_graphicsContext->CaptureRenderTargetTex(dstTexture);
	}

	void Graphics::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
		g_graphicsContext->SetViewport(x, y, width, height);
	}

	void Graphics::GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height) {
		g_graphicsContext->GetViewport(x, y, width, height);
	}

	void Graphics::SetClearColor(float r, float g, float b, float a) {
		g_graphicsContext->SetClearColor(r, g, b, a);
	}

	void Graphics::Resize(int32_t width, int32_t height) {
		g_graphicsContext->Resize(width, height);
	}

	GraphicsContext& Graphics::GetGraphicsContext() {
		return *g_graphicsContext;
	}
}