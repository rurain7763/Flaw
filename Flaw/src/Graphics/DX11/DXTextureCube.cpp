#include "pch.h"
#include "DXType.h"
#include "DXContext.h"
#include "DXTextures.h"
#include "Graphics/GraphicsFunc.h"
#include "Log/Log.h"

namespace flaw {
	DXTextureCube::DXTextureCube(DXContext& context, const Descriptor& descriptor)
		: _context(context)
	{
		if (!CreateTexture(descriptor)) {
			Log::Error("CreateTexture failed");
			return;
		}

		if (descriptor.bindFlags & BindFlag::RenderTarget && !CreateRenderTargetView(descriptor.format)) {
			Log::Error("CreateRenderTargetView failed");
			return;
		}

		if (descriptor.bindFlags & BindFlag::DepthStencil && !CreateDepthStencilView(descriptor.format)) {
			Log::Error("CreateDepthStencilView failed");
			return;
		}

		if (descriptor.bindFlags & BindFlag::ShaderResource && !CreateShaderResourceView(descriptor.format)) {
			Log::Error("CreateShaderResourceView failed");
			return;
		}
	}

	bool DXTextureCube::CreateTexture(const Descriptor& descriptor) {
		if (!descriptor.data) {
			Log::Error("DXTextureCube::CreateTexture: data is null");
			return false;
		}

		uint32_t faceWidth = descriptor.width;
		uint32_t faceHeight = descriptor.height;

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = faceWidth;
		desc.Height = faceHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 6; // 6 faces for cube map
		desc.Format = ConvertToDXGIFormat(descriptor.format);
		desc.SampleDesc.Count = 1;
		desc.Usage = ConvertD3D11Usage(descriptor.usage);
		desc.BindFlags = ConvertD3D11Bind(descriptor.bindFlags);
		desc.CPUAccessFlags = ConvertD3D11Access(descriptor.access);
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		if (descriptor.data) {
			uint32_t sizePerPixel = GetSizePerPixel(descriptor.format);
			uint32_t rowPitch = faceWidth * sizePerPixel;
			uint32_t fullRowPitch = descriptor.width * sizePerPixel;

			D3D11_SUBRESOURCE_DATA initData[6];
			std::vector<uint8_t> dataBuffer[6];

			struct FacePos { int32_t x, y; };
			FacePos faceCoords[6];

			if (descriptor.layout == Layout::Horizontal) {
				// +---- +---- +---- +---- +---- +---- +
				//| +X || -X || +Y || -Y || +Z || -Z |
				//+---- +---- +---- +---- +---- +---- +
				faceWidth = descriptor.width / 6;

				faceCoords[0] = { 0, 0 }; // +X
				faceCoords[1] = { 1, 0 }; // -X
				faceCoords[2] = { 2, 0 }; // +Y
				faceCoords[3] = { 3, 0 }; // -Y
				faceCoords[4] = { 4, 0 }; // +Z
				faceCoords[5] = { 5, 0 };  // -Z
			}
			else if (descriptor.layout == Layout::HorizontalCross) {
				//		+---- +
				//		| +Y |
				//+----++----++----++---- +
				//| -X || +Z || +X || -Z |
				//+----++----++----++---- +
				//		| -Y |
				//		+---- +
				faceWidth = descriptor.width / 4;
				faceHeight = descriptor.height / 3;

				faceCoords[0] = { 2, 1 }; // +X
				faceCoords[1] = { 0, 1 }; // -X
				faceCoords[2] = { 1, 0 }; // +Y
				faceCoords[3] = { 1, 2 }; // -Y
				faceCoords[4] = { 1, 1 }; // +Z
				faceCoords[5] = { 3, 1 }; // -Z
			}

			for (uint32_t i = 0; i < 6; ++i) {
				const FacePos& faceCoord = faceCoords[i];

				std::vector<uint8_t>& data = dataBuffer[i];
				data.resize(faceWidth * faceHeight * sizePerPixel);

				const uint8_t* src = (const uint8_t*)descriptor.data +
					(faceCoord.y * faceHeight * fullRowPitch) +
					(faceCoord.x * faceWidth * sizePerPixel);

				for (uint32_t y = 0; y < faceHeight; ++y) {
					memcpy(data.data() + y * rowPitch, src, rowPitch);
					src += fullRowPitch;
				}

				initData[i].pSysMem = data.data();
				initData[i].SysMemPitch = rowPitch;
				initData[i].SysMemSlicePitch = 0;
			}

			if (FAILED(_context.Device()->CreateTexture2D(&desc, initData, _texture.GetAddressOf()))) {
				Log::Error("DXTextureCube::CreateTexture: CreateTexture2D failed");
				return false;
			}
		}
		else {
			if (FAILED(_context.Device()->CreateTexture2D(&desc, nullptr, _texture.GetAddressOf()))) {
				Log::Error("DXTextureCube::CreateTexture: CreateTexture2D failed");
				return false;
			}
		}

		_format = descriptor.format;
		_usage = descriptor.usage;
		_acessFlags = descriptor.access;
		_bindFlags = descriptor.bindFlags;
		_width = descriptor.width;
		_height = descriptor.height;

		return true;
	}

	bool DXTextureCube::CreateRenderTargetView(const PixelFormat format) {
		D3D11_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = ConvertToDXGIFormat(format);
		rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = 0;
		rtvDesc.Texture2DArray.ArraySize = 6;

		if (FAILED(_context.Device()->CreateRenderTargetView(_texture.Get(), &rtvDesc, _rtv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTextureCube::CreateDepthStencilView(const PixelFormat format) {
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = ConvertToDXGIFormat(format);
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = 0;
		dsvDesc.Texture2DArray.ArraySize = 6;

		if (FAILED(_context.Device()->CreateDepthStencilView(_texture.Get(), &dsvDesc, _dsv.GetAddressOf()))) {
			return false;
		}

		return true;
	}

	bool DXTextureCube::CreateShaderResourceView(const PixelFormat format) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = ConvertToDXGIFormat(format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}