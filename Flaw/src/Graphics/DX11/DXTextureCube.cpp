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

		if (!CreateShaderResourceView(descriptor.format)) {
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

		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = faceWidth;
		desc.Height = faceHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 6; // 6 faces for cube map
		desc.Format = GetFormat(descriptor.format);
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

		uint32_t sizePerPixel = GetSizePerPixel(descriptor.format);
		uint32_t rowPitch = faceWidth * sizePerPixel;
		uint32_t fullRowPitch = descriptor.width * sizePerPixel;

		D3D11_SUBRESOURCE_DATA initData[6];
		std::vector<uint8_t> dataBuffer[6];

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

		_format = descriptor.format;

		return true;
	}

	void DXTextureCube::BindToGraphicsShader(const uint32_t slot) {
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

	void DXTextureCube::BindToComputeShader(const BindFlag bindFlags, const uint32_t slot) {
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

	void DXTextureCube::Unbind() {
		if (_unbindFunc) {
			_unbindFunc();
		}
	}

	bool DXTextureCube::CreateShaderResourceView(const PixelFormat format) {
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = GetFormat(format);
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MipLevels = 1;
		srvDesc.TextureCube.MostDetailedMip = 0;

		if (FAILED(_context.Device()->CreateShaderResourceView(_texture.Get(), &srvDesc, _srv.GetAddressOf()))) {
			return false;
		}

		return true;
	}
}