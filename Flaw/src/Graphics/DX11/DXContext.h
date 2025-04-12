#pragma once

#include "Core.h"

#include "Graphics/GraphicsContext.h"
#include "Graphics/GraphicsCommandQueue.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <array>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class FAPI DXContext : public GraphicsContext {
	public:
		DXContext(PlatformContext& context, int32_t width, int32_t height);
		~DXContext();

		void Prepare() override;
		void Present() override;

		Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) override;
		Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) override;
		Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) override;
		Ref<GraphicsPipeline> CreateGraphicsPipeline() override;

		Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size) override;

		Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) override;

		Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor) override;
		Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor) override;

		void SetRenderTexture(uint32_t slot, Ref<Texture2D> texture, float clearValue[4]) override;
		void ResetRenderTexture(uint32_t slot) override;

		GraphicsCommandQueue& GetCommandQueue() override;

		void CaptureRenderTargetTex(Ref<Texture2D>& dstTexture) override;

		void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) override;
		void GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height) override;

		void SetClearColor(float r, float g, float b, float a) override;

		void Resize(int32_t width, int32_t height) override;
		void GetSize(int32_t& width, int32_t& height) override;

		Ref<ComputeShader> CreateComputeShader(const char* filename) override;
		Ref<ComputePipeline> CreateComputePipeline() override;

		inline ComPtr<ID3D11Device> Device() const { return _device; }
		inline ComPtr<ID3D11DeviceContext> DeviceContext() const { return _deviceContext; }

	private:
		int32_t CreateSwapChain();
		int32_t CreateRenderTarget();
		int32_t CreateDepthStencil();

		ID3D11SamplerState* CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v);

		void SetVSync(bool enable);

	private:
		HWND _hWnd;

		float _swapInterval;

		D3D11_VIEWPORT _viewPort;

		// rendering size
		int32_t _renderWidth, _renderHeight;

		ComPtr<ID3D11Device> _device;
		ComPtr<ID3D11DeviceContext> _deviceContext;

		ComPtr<IDXGISwapChain> _swapChain;
		ComPtr<ID3D11Texture2D> _renderTarget;
		ComPtr<ID3D11RenderTargetView> _renderTargetView;
		float _clearColor[4];

		// additional render target, limit 7
		struct RenderTarget {
			ComPtr<ID3D11RenderTargetView> rtv = nullptr;
			float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		};

		std::array<RenderTarget, 7> _additionalRenderTargets;

		ComPtr<ID3D11Texture2D> _depthStencil;
		ComPtr<ID3D11DepthStencilView> _depthStencilView;

		std::array<ID3D11SamplerState*, 1> _samplerStates;

		Ref<GraphicsCommandQueue> _commandQueue;
	};
}