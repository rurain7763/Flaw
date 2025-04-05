#include "pch.h"
#include "DXTextures.h"
#include "DXContext.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

namespace flaw {
	DXTexture2D::DXTexture2D(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		if (!CreateTexture(descriptor)) {
			Log::Error("CreateTexture failed");
			return;
		}

		if (descriptor.bindFlags & BindFlag::RenderTarget) {
			if (!CreateRenderTargetView(descriptor.format)) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}

		if (descriptor.bindFlags & BindFlag::DepthStencil) {
			if (!CreateDepthStencilView(descriptor.format)) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}

		if (descriptor.bindFlags & BindFlag::ShaderResource) {
			if (!CreateShaderResourceView(descriptor.format)) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}
		}

		if (descriptor.bindFlags & BindFlag::UnorderedAccess) {
			if (!CreateUnorderedAccessView(descriptor.format)) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	DXTexture2D::DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, const uint32_t bindFlags)
		: _context(context)
	{
		_texture = texture;

		if (bindFlags & BindFlag::RenderTarget) {
			if (!CreateRenderTargetView(format)) {
				Log::Error("CreateRenderTargetView failed");
				return;
			}
		}
		if (bindFlags & BindFlag::DepthStencil) {
			if (!CreateDepthStencilView(format)) {
				Log::Error("CreateDepthStencilView failed");
				return;
			}
		}
		if (bindFlags & BindFlag::ShaderResource) {
			if (!CreateShaderResourceView(format)) {
				Log::Error("CreateShaderResourceView failed");
				return;
			}
		}
		if (bindFlags & BindFlag::UnorderedAccess) {
			if (!CreateUnorderedAccessView(format)) {
				Log::Error("CreateUnorderedAccessView failed");
				return;
			}
		}
	}

	void DXTexture2D::BindToGraphicsShader(const uint32_t slot) {
		if (!_srv) {
			Log::Warn("ShaderResourceView is nullptr");
			return;
		}

		_context.DeviceContext()->PSSetShaderResources(slot, 1, _srv.GetAddressOf());

		_unbindFunc = [this, slot]() {
			ID3D11ShaderResourceView* nullSRV = nullptr;
			_context.DeviceContext()->PSSetShaderResources(slot, 1, &nullSRV);
			_unbindFunc = nullptr;
		};
	}

	void DXTexture2D::BindToComputeShader(const BindFlag bindFlags, const uint32_t slot) {
		if (bindFlags & BindFlag::ShaderResource) {
			if (!_srv) {
				Log::Warn("ShaderResourceView is nullptr");
				return;
			}

			_context.DeviceContext()->CSSetShaderResources(slot, 1, _srv.GetAddressOf());

			_unbindFunc = [this, slot]() {
				ID3D11ShaderResourceView* nullSRV = nullptr;
				_context.DeviceContext()->CSSetShaderResources(slot, 1, &nullSRV);
				_unbindFunc = nullptr;
			};
		}
		else if (bindFlags & BindFlag::UnorderedAccess) {
			if (!_uav) {
				Log::Warn("UnorderedAccessView is nullptr");
				return;
			}

			_context.DeviceContext()->CSSetUnorderedAccessViews(slot, 1, _uav.GetAddressOf(), nullptr);

			_unbindFunc = [this, slot]() {
				ID3D11UnorderedAccessView* nullUAV = nullptr;
				_context.DeviceContext()->CSSetUnorderedAccessViews(slot, 1, &nullUAV, nullptr);
				_unbindFunc = nullptr;
			};
		}
	}

	void DXTexture2D::Unbind() {
		if (_unbindFunc) {
			_unbindFunc();
		}
	}

	void DXTexture2D::Fetch(void* outData, const uint32_t size) const {
		if (_acessFlags & AccessFlag::Read) {
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (FAILED(_context.DeviceContext()->Map(_texture.Get(), 0, D3D11_MAP_READ, 0, &mappedResource))) {
				Log::Error("Map failed");
				return;
			}
			memcpy(outData, mappedResource.pData, size);
			_context.DeviceContext()->Unmap(_texture.Get(), 0);
		}
		else {
			Log::Error("Texture is not readable");
		}
	}

	void DXTexture2D::CopyTo(Ref<Texture2D>& target) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(target);
		_context.DeviceContext()->CopyResource(dxTexture->GetNativeTexture().Get(), _texture.Get());
	}

	void DXTexture2D::CopyToSub(Ref<Texture2D>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const {
		auto dxTexture = std::static_pointer_cast<DXTexture2D>(target);

		D3D11_BOX box = {};
		box.left = x;
		box.top = y;
		box.right = x + width;
		box.bottom = y + height;
		box.front = 0;
		box.back = 1;

		_context.DeviceContext()->CopySubresourceRegion(
			dxTexture->GetNativeTexture().Get(), 
			0, // dst mip level 
			0, 0, 0, // dst x, y, z
			_texture.Get(), 
			0, // src mip level	
			&box
		);
	}

	bool DXTexture2D::CreateTexture(const Descriptor& descriptor) {
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = descriptor.width;
		desc.Height = descriptor.height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = GetFormat(descriptor.format);
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = GetUsage(descriptor.usage);
		desc.CPUAccessFlags = GetAccessFlag(descriptor.access);
		desc.BindFlags = GetBindFlags(descriptor.bindFlags);
		desc.MiscFlags = 0;

		if (descriptor.data) {
			D3D11_SUBRESOURCE_DATA initData = {};
			initData.pSysMem = descriptor.data;
			initData.SysMemPitch = descriptor.width * GetSizePerPixel(descriptor.format);
			initData.SysMemSlicePitch = 0;

			if (FAILED(_context.Device()->CreateTexture2D(&desc, &initData, _texture.GetAddressOf()))) {
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
				return false;
			}
		}

		_format = descriptor.format;
		_usage = descriptor.usage;
		_acessFlags = descriptor.access;

		_width = descriptor.width;
		_height = descriptor.height;

		return true;
	}

	bool DXTexture2D::CreateRenderTargetView(const PixelFormat format) {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = GetFormat(format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, _rtv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateDepthStencilView(const PixelFormat format) {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = GetFormat(format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;

		if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateShaderResourceView(const PixelFormat format) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = GetFormat(format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = 1;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTexture2D::CreateUnorderedAccessView(const PixelFormat format) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = GetFormat(format);
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

		if (FAILED(_context.Device()->CreateUnorderedAccessView(_texture.Get(), &uavDesc, _uav.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	DXGI_FORMAT DXTexture2D::GetFormat(PixelFormat format) {
		switch (format) {
		case PixelFormat::BGR8:
			return DXGI_FORMAT_B8G8R8X8_UNORM;
		case PixelFormat::RGBA8:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case PixelFormat::RGBA32F:
			return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case PixelFormat::RG8:
			return DXGI_FORMAT_R8G8_UNORM;
		case PixelFormat::R8:
			return DXGI_FORMAT_R8_UNORM;
		case PixelFormat::R32_UINT:
			return DXGI_FORMAT_R32_UINT;
		case PixelFormat::D24S8_UINT:
			return DXGI_FORMAT_D24_UNORM_S8_UINT;
		default:
			throw std::runtime_error("Unknown pixel format");
		}
	}

	D3D11_USAGE DXTexture2D::GetUsage(const UsageFlag usage) {
		switch (usage)
		{
		case UsageFlag::Static:
			return D3D11_USAGE_DEFAULT;
		case UsageFlag::Dynamic:
			return D3D11_USAGE_DYNAMIC;
		case UsageFlag::Staging:
			return D3D11_USAGE_STAGING;
		}

		throw std::runtime_error("Unknown usage flag");
	}

	uint32_t DXTexture2D::GetAccessFlag(const uint32_t access) {
		uint32_t accessFlag = 0;
		if (access & AccessFlag::Write) {
			accessFlag |= D3D11_CPU_ACCESS_WRITE;
		}
		if (access & AccessFlag::Read) {
			accessFlag |= D3D11_CPU_ACCESS_READ;
		}
		return accessFlag;
	}

	uint32_t DXTexture2D::GetBindFlags(uint32_t flags) {
		uint32_t bindFlags = 0;

		if (flags & BindFlag::ShaderResource) {
			bindFlags |= D3D11_BIND_SHADER_RESOURCE;
		}

		if (flags & BindFlag::RenderTarget) {
			bindFlags |= D3D11_BIND_RENDER_TARGET;
		}

		if (flags & BindFlag::DepthStencil) {
			bindFlags |= D3D11_BIND_DEPTH_STENCIL;
		}

		if (flags & BindFlag::UnorderedAccess) {
			bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
		}

		return bindFlags;
	}
}

