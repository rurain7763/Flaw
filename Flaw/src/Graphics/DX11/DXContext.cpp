#include "pch.h"
#include "DXContext.h"
#include "Platform/Windows/WindowsContext.h"
#include "Log/Log.h"
#include "DXVertexBuffer.h"
#include "DXIndexBuffer.h"
#include "DXGraphicsShader.h"
#include "DXGraphicsPipeline.h"
#include "DXCommandQueue.h"
#include "DXConstantBuffer.h"
#include "DXStructuredBuffer.h"
#include "DXTextures.h"

namespace flaw {
	DXContext::DXContext(PlatformContext& context, int32_t width, int32_t height) {
		auto* wndContext = dynamic_cast<WindowsContext*>(&context);
		if (!wndContext) {
			Log::Fatal("DXContext::Initialize failed: Platform context is not WindowsContext");
			return;
		}

		_hWnd = wndContext->GetWindowHandle();
		_renderWidth = width;
		_renderHeight = height;

		UINT createDeviceFlags = 0;
#if _DEBUG
		createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

		D3D_FEATURE_LEVEL featureLevel;

		if (FAILED(D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&_device,
			&featureLevel,
			&_deviceContext))) 
		{
			Log::Fatal("D3D11CreateDevice failed");
			return;
		}

		if (CreateSwapChain()) {
			return;
		}

		if (CreateRenderTarget()) {
			return;
		}

		if (CreateDepthStencil()) {
			return;
		}

		ID3D11SamplerState* samplerState = CreateSamplerState(
			D3D11_FILTER_MIN_MAG_MIP_LINEAR,
			//D3D11_FILTER_ANISOTROPIC, 
			//D3D11_FILTER_MIN_MAG_MIP_POINT,
			D3D11_TEXTURE_ADDRESS_WRAP,
			D3D11_TEXTURE_ADDRESS_WRAP
		);

		if (!samplerState) {
			return;
		}

		_samplerStates[0] = samplerState;

		_deviceContext->PSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->VSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->GSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->HSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->DSSetSamplers(0, _samplerStates.size(), _samplerStates.data());
		_deviceContext->CSSetSamplers(0, _samplerStates.size(), _samplerStates.data());

		_commandQueue = CreateRef<DXCommandQueue>(*this);

		Log::Info("DirectX 11 Initialized");

		SetViewport(0, 0, _renderWidth, _renderHeight);
		SetVSync(false);
	}

	DXContext::~DXContext() {
		for (auto samplerState : _samplerStates) {
			samplerState->Release();
		}
	}

	void DXContext::Prepare() {
		std::vector<ID3D11RenderTargetView*> renderTargets;

		_deviceContext->ClearRenderTargetView(_renderTargetView.Get(), _clearColor);
		renderTargets.push_back(_renderTargetView.Get());

		for (auto& rt : _additionalRenderTargets) {
			if (rt.rtv) {
				_deviceContext->ClearRenderTargetView(rt.rtv.Get(), rt.clearColor);
				renderTargets.push_back(rt.rtv.Get());
			}
		}

		_deviceContext->ClearDepthStencilView(_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		_deviceContext->OMSetRenderTargets(renderTargets.size(), renderTargets.data(), _depthStencilView.Get());

		_deviceContext->RSSetViewports(1, &_viewPort);
	}

	void DXContext::Present() {
		_swapChain->Present(_swapInterval, 0);
	}

	Ref<VertexBuffer> DXContext::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
		return CreateRef<DXVertexBuffer>(*this, descriptor);
	}

	Ref<IndexBuffer> DXContext::CreateIndexBuffer() {
		return CreateIndexBuffer(IndexBuffer::Descriptor());
	}

	Ref<IndexBuffer> DXContext::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
		return CreateRef<DXIndexBuffer>(*this, descriptor);
	}

	Ref<GraphicsShader> DXContext::CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) {
		return CreateRef<DXGraphicsShader>(*this, filePath, compileFlag);
	}

	Ref<GraphicsPipeline> DXContext::CreateGraphicsPipeline() {
		return CreateRef<DXGraphicsPipeline>(*this);
	}

	Ref<ConstantBuffer> DXContext::CreateConstantBuffer(uint32_t size) {
		return CreateRef<DXConstantBuffer>(*this, size);
	}

	Ref<StructuredBuffer> DXContext::CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) {
		return CreateRef<DXStructuredBuffer>(*this, desc);
	}

	Ref<Texture2D> DXContext::CreateTexture2D(const Texture2D::Descriptor& descriptor) {
		return CreateRef<DXTexture2D>(*this, descriptor);
	}

	void DXContext::SetRenderTexture(uint32_t slot, Ref<Texture2D> texture, float clearValue[4]) {
		if (slot == 0) {
			Log::Error("Cannot set render target to slot 0, it is reserved for main render target");
			return;
		}
		
		slot = slot - 1;

		auto dxTexture = std::static_pointer_cast<DXTexture2D>(texture);

		auto rtv = dxTexture->GetRenderTargetView();
		if (!rtv) {
			Log::Error("RenderTargetView is null");
			return;
		}

		_additionalRenderTargets[slot] = RenderTarget{ rtv, clearValue[0], clearValue[1], clearValue[2], clearValue[3] };
	}

	void DXContext::ResetRenderTexture(uint32_t slot) {
		_additionalRenderTargets[slot] = RenderTarget{ nullptr, 0.0f, 0.0f, 0.0f, 0.0f };
	}

	GraphicsCommandQueue& DXContext::GetCommandQueue() {
		return *_commandQueue;
	}

	void DXContext::SetViewport(int32_t x, int32_t y, int32_t width, int32_t height) {
		_viewPort.TopLeftX = x;
		_viewPort.TopLeftY = y;
		_viewPort.Width = static_cast<float>(width);
		_viewPort.Height = static_cast<float>(height);

		// depth range
		_viewPort.MinDepth = 0.0f;
		_viewPort.MaxDepth = 1.0f;
	}

	void DXContext::GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height) {
		x = static_cast<int32_t>(_viewPort.TopLeftX);
		y = static_cast<int32_t>(_viewPort.TopLeftY);
		width = static_cast<int32_t>(_viewPort.Width);
		height = static_cast<int32_t>(_viewPort.Height);
	}

	void DXContext::SetClearColor(float r, float g, float b, float a) {
		_clearColor[0] = r;
		_clearColor[1] = g;
		_clearColor[2] = b;
		_clearColor[3] = a;
	}

	void DXContext::Resize(int32_t width, int32_t height) {
		if (!_swapChain) {
			return;
		}

		if (width == 0 || height == 0) {
			return;
		}

		_deviceContext->OMSetRenderTargets(0, nullptr, nullptr); // 기존 RenderTarget 해제

		_renderTarget.Reset();
		_renderTargetView.Reset();

		_depthStencil.Reset();
		_depthStencilView.Reset();

		if (FAILED(_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0))) {
			Log::Error("ResizeBuffers failed");
			return;
		}

		_renderWidth = width;
		_renderHeight = height;

		if (CreateRenderTarget()) {
			Log::Error("CreateRenderTarget failed");
			return;
		}

		if (CreateDepthStencil()) {
			Log::Error("CreateDepthStencil failed");
			return;
		}
	}

	void DXContext::CaptureRenderTargetTex(Ref<Texture2D>& dstTexture) {
		Ref<DXTexture2D> casted = std::static_pointer_cast<DXTexture2D>(dstTexture);
		_deviceContext->CopyResource(casted->GetNativeTexture().Get(), _renderTarget.Get());
	}

	int32_t DXContext::CreateSwapChain() {
		ComPtr<IDXGIDevice> dxgiDevice = nullptr;
		ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
		ComPtr<IDXGIFactory1> dxgiFactory = nullptr;

		_device->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgiDevice.GetAddressOf());
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)dxgiAdapter.GetAddressOf());
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)dxgiFactory.GetAddressOf());

		DXGI_SWAP_CHAIN_DESC swapchainDesc = {};
		swapchainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferCount = 1;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.BufferDesc.Width = _renderWidth;
		swapchainDesc.BufferDesc.Height = _renderHeight;
		swapchainDesc.OutputWindow = _hWnd;
		swapchainDesc.Windowed = TRUE; // TODO: 수정 가능하도록 변경
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapchainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapchainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapchainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapchainDesc.SampleDesc.Count = 1;
		swapchainDesc.SampleDesc.Quality = 0;

		if (FAILED(dxgiFactory->CreateSwapChain(_device.Get(), &swapchainDesc, _swapChain.GetAddressOf()))) {
			Log::Error("CreateSwapChain failed");
			return -1;
		}

		return 0;
	}

	int32_t DXContext::CreateRenderTarget() {
		if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)_renderTarget.GetAddressOf()))) {
			Log::Error("GetBuffer failed");
			return -1;
		}

		if (FAILED(_device->CreateRenderTargetView(_renderTarget.Get(), nullptr, _renderTargetView.GetAddressOf()))) {
			Log::Error("CreateRenderTargetView failed");
			return -1;
		}

		return 0;
	}

	int32_t DXContext::CreateDepthStencil() {
		D3D11_TEXTURE2D_DESC dsDesc = {};
		dsDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // depth 24bit, stencil 8bit
		dsDesc.Width = _renderWidth;
		dsDesc.Height = _renderHeight;

		dsDesc.ArraySize = 1;
		dsDesc.CPUAccessFlags = 0;
		dsDesc.MipLevels = 1;
		dsDesc.Usage = D3D11_USAGE_DEFAULT;
		dsDesc.SampleDesc.Count = 1;
		dsDesc.SampleDesc.Quality = 0;
		dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		if (FAILED(_device->CreateTexture2D(&dsDesc, nullptr, _depthStencil.GetAddressOf()))) {
			Log::Error("CreateTexture2D failed");
			return -1;
		}

		if (FAILED(_device->CreateDepthStencilView(_depthStencil.Get(), nullptr, _depthStencilView.GetAddressOf()))) {
			Log::Error("CreateDepthStencilView failed");
			return -1;
		}

		return 0;
	}

	ID3D11SamplerState* DXContext::CreateSamplerState(D3D11_FILTER filter, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v) {
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = filter;
		samplerDesc.AddressU = u;
		samplerDesc.AddressV = v;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

		ID3D11SamplerState* samplerState;
		if (FAILED(_device->CreateSamplerState(&samplerDesc, &samplerState))) {
			Log::Error("CreateSamplerState failed");
			return nullptr;
		}

		return samplerState;
	}

	void DXContext::SetVSync(bool enable) {
		_swapInterval = enable ? 1 : 0;
	}
}
