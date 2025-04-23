#include "pch.h"
#include "Graphics.h"
#include "Platform.h"

#define NOMINMAX
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

		auto mainMrt = Graphics::GetMainRenderPass();

		Texture2D::Descriptor desc = {};
		desc.width = width;
		desc.height = height;
		desc.format = PixelFormat::R32_UINT;
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::RenderTarget;

		GraphicsRenderTarget renderTarget = {};
		renderTarget.blendMode = BlendMode::Disabled;
		renderTarget.texture = g_graphicsContext->CreateTexture2D(desc);
		renderTarget.clearValue = { (float)std::numeric_limits<int32_t>().max(), 0.0f, 0.0f, 0.0f };

		mainMrt->PushRenderTarget(renderTarget);
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

	Ref<TextureCube> Graphics::CreateTextureCube(const TextureCube::Descriptor& descriptor) {
		return g_graphicsContext->CreateTextureCube(descriptor);
	}

	Ref<GraphicsRenderPass> Graphics::GetMainRenderPass() {
		return g_graphicsContext->GetMainRenderPass();
	}

	GraphicsCommandQueue& Graphics::GetCommandQueue() {
		return g_graphicsContext->GetCommandQueue();
	}

	Ref<GraphicsRenderPass> Graphics::CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) {
		return g_graphicsContext->CreateRenderPass(desc);
	}

	void Graphics::SetRenderPass(GraphicsRenderPass* renderPass) {
		g_graphicsContext->SetRenderPass(renderPass);
	}

	void Graphics::ResetRenderPass() {
		g_graphicsContext->ResetRenderPass();
	}

	void Graphics::Resize(int32_t width, int32_t height) {
		g_graphicsContext->Resize(width, height);
	}

	void Graphics::GetSize(int32_t& width, int32_t& height) {
		g_graphicsContext->GetSize(width, height);
	}

	Ref<ComputeShader> Graphics::CreateComputeShader(const char* filePath) {
		return g_graphicsContext->CreateComputeShader(filePath);
	}

	Ref<ComputePipeline> Graphics::CreateComputePipeline() {
		return g_graphicsContext->CreateComputePipeline();
	}

	GraphicsContext& Graphics::GetGraphicsContext() {
		return *g_graphicsContext;
	}
}