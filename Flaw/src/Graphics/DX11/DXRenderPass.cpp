#include "pch.h"
#include "DXRenderPass.h"
#include "DXContext.h"
#include "DXTextures.h"
#include "Log/Log.h"
#include "DXType.h"

namespace flaw {
	DXRenderPass::DXRenderPass(DXContext& context, const Descriptor& desc)
		: _context(context) 
	{
		if (desc.renderTargets.size() > MaxRenderTargets) {
			Log::Error("DXMultiRenderTarget::DXMultiRenderTarget: Too many render targets. Max is %d.", MaxRenderTargets);
			return;
		}

		_renderTargets = desc.renderTargets;
		_depthStencil = desc.depthStencil;

		CreateBlendState();
	}

	DXRenderPass::~DXRenderPass() {
		Unbind();
	}

	void DXRenderPass::Bind(bool clearColor, bool clearDepthStencil) {
		int32_t width, height;
		_context.GetSize(width, height);

		Resize(width, height);

		if (clearColor) {
			ClearAllRenderTargets();
		}

		if (clearDepthStencil) {
			ClearDepthStencil();
		}

		_context.SetRenderPass(this);

		std::vector<ID3D11RenderTargetView*> rtvArray(_renderTargets.size());
		std::vector<D3D11_VIEWPORT> viewports(_renderTargets.size());
		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto& renderTarget = _renderTargets[i];
			
			rtvArray[i] = std::static_pointer_cast<DXTexture2D>(renderTarget.texture)->GetRenderTargetView().Get();

			auto& viewPort = viewports[i];
			if (renderTarget.viewportFunc) {
				renderTarget.viewportFunc(viewPort.TopLeftX, viewPort.TopLeftY, viewPort.Width, viewPort.Height);
			}
			else {
				viewPort.TopLeftX = 0;
				viewPort.TopLeftY = 0;
				viewPort.Width = static_cast<float>(width);
				viewPort.Height = static_cast<float>(height);
			}

			viewPort.MinDepth = 0.0f;
			viewPort.MaxDepth = 1.0f;
		}

		ComPtr<ID3D11DepthStencilView> depthStencilView = nullptr;
		if (_depthStencil.texture) {
			depthStencilView = std::static_pointer_cast<DXTexture2D>(_depthStencil.texture)->GetDepthStencilView().Get();
		}

		_context.DeviceContext()->OMSetRenderTargets(static_cast<UINT>(rtvArray.size()), rtvArray.data(), depthStencilView.Get());
		_context.DeviceContext()->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
		_context.DeviceContext()->OMSetBlendState(_blendState.Get(), nullptr, 0xffffffff);
	}

	void DXRenderPass::Unbind() {
		if (_context.GetRenderPass() != this) {
			return;
		}

		_context.ResetRenderPass();
	}

	void DXRenderPass::Resize(int32_t width, int32_t height) {
		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto& renderTarget = _renderTargets[i];

			if (renderTarget.texture == nullptr) {
				continue;
			}

			if (renderTarget.texture->GetWidth() == width && renderTarget.texture->GetHeight() == height) {
				continue;
			}

			if (renderTarget.resizeFunc) {
				renderTarget.texture.reset();
				renderTarget.texture = renderTarget.resizeFunc(width, height);
			}
			else {
				Texture2D::Descriptor texDesc = {};
				texDesc.width = width;
				texDesc.height = height;
				texDesc.format = renderTarget.texture->GetPixelFormat();
				texDesc.usage = renderTarget.texture->GetUsage();
				texDesc.bindFlags = renderTarget.texture->GetBindFlags();

				renderTarget.texture.reset();
				renderTarget.texture = _context.CreateTexture2D(texDesc);
			}
		}

		if (_depthStencil.texture == nullptr) {
			return;
		}

		if (_depthStencil.texture->GetWidth() == width && _depthStencil.texture->GetHeight() == height) {
			return;
		}

		if (_depthStencil.resizeFunc) {
			_depthStencil.texture.reset();
			_depthStencil.texture = _depthStencil.resizeFunc(width, height);
		}
		else {
			Texture2D::Descriptor depthTexDesc = {};
			depthTexDesc.width = width;
			depthTexDesc.height = height;
			depthTexDesc.format = _depthStencil.texture->GetPixelFormat();
			depthTexDesc.usage = _depthStencil.texture->GetUsage();
			depthTexDesc.bindFlags = _depthStencil.texture->GetBindFlags();

			_depthStencil.texture.reset();
			_depthStencil.texture = _context.CreateTexture2D(depthTexDesc);
		}
	}

	void DXRenderPass::PushRenderTarget(const GraphicsRenderTarget& renderTarget) {
		if (_renderTargets.size() >= MaxRenderTargets) {
			Log::Error("DXMultiRenderTarget::PushRenderTarget: Max render targets reached");
			return;
		}
		_renderTargets.push_back(renderTarget);
		CreateBlendState();
	}

	void DXRenderPass::PopRenderTarget() {
		if (_renderTargets.size() <= 1) {
			Log::Error("DXMultiRenderTarget::PopRenderTarget: No render targets to pop");
			return;
		}
		_renderTargets.pop_back();
		CreateBlendState();
	}

	void DXRenderPass::SetBlendMode(int32_t slot, BlendMode blendMode, bool alphaToCoverage) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");

		auto& renderTarget = _renderTargets[slot];

		if (renderTarget.blendMode == blendMode && renderTarget.alphaToCoverage == alphaToCoverage) {
			return;
		}

		renderTarget.blendMode = blendMode;
		renderTarget.alphaToCoverage = alphaToCoverage;

		CreateBlendState();
	}

	Ref<Texture2D> DXRenderPass::GetRenderTargetTex(int32_t slot) {
		FASSERT(slot >= 0 && slot < _renderTargets.size(), "Invalid render target slot");
		return _renderTargets[slot].texture;
	}

	Ref<Texture2D> DXRenderPass::GetDepthStencilTex() {
		return _depthStencil.texture;
	}

	void DXRenderPass::ClearAllRenderTargets() {
		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(_renderTargets[i].texture);
			_context.DeviceContext()->ClearRenderTargetView(dxTexture->GetRenderTargetView().Get(), _renderTargets[i].clearValue.data());
		}
	}

	void DXRenderPass::ClearDepthStencil() {
		if (_depthStencil.texture) {
			auto dxTexture = std::static_pointer_cast<DXTexture2D>(_depthStencil.texture);
			_context.DeviceContext()->ClearDepthStencilView(dxTexture->GetDepthStencilView().Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}

	void DXRenderPass::CreateBlendState() {
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.IndependentBlendEnable = TRUE;

		for (int32_t i = 0; i < _renderTargets.size(); ++i) {
			auto& renderTarget = _renderTargets[i];
			auto& renderTargetDesc = blendDesc.RenderTarget[i];

			if (renderTarget.blendMode == BlendMode::Disabled) {
				renderTargetDesc.BlendEnable = FALSE;
				renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
				continue;
			}

			renderTargetDesc.BlendEnable = TRUE;
			ConvertD3D11Blend(renderTarget.blendMode, renderTargetDesc.SrcBlend, renderTargetDesc.DestBlend);
			renderTargetDesc.BlendOp = D3D11_BLEND_OP_ADD;
			renderTargetDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
			renderTargetDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
			renderTargetDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
			renderTargetDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

			blendDesc.AlphaToCoverageEnable |= renderTarget.alphaToCoverage;
		}

		if (FAILED(_context.Device()->CreateBlendState(&blendDesc, &_blendState))) {
			Log::Error("DXMultiRenderTarget::CreateBlendState: Failed to create blend state");
			return;
		}
	}
}