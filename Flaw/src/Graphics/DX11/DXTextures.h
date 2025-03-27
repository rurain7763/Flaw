#pragma once

#include "Graphics/Texture.h"

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
	class DXContext;

	class DXTexture2D : public Texture {
	public:
		DXTexture2D(DXContext& context, const Descriptor& descriptor);
		DXTexture2D(DXContext& context, const ComPtr<ID3D11Texture2D>& texture, const PixelFormat format, const uint32_t bindFlags);

		void BindToGraphicsShader(const uint32_t slot) override;
		void BindToComputeShader(const BindFlag bindFlags, const uint32_t slot) override;

		void Unbind() override;

		void Fetch(void* outData, const uint32_t size) const override;

		void CopyTo(Ref<Texture>& target) const override;
		void CopyToSub(Ref<Texture>& target, const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) const override;

		uint32_t GetWidth() const override { return _width; }
		uint32_t GetHeight() const override { return _height; }

		ComPtr<ID3D11Texture2D> GetNativeTexture() const { return _texture; }
		ComPtr<ID3D11ShaderResourceView> GetShaderResourceView() const { return _srv; }
		ComPtr<ID3D11RenderTargetView> GetRenderTargetView() const { return _rtv; }

	private:
		bool CreateTexture(const Descriptor& descriptor);

		bool CreateRenderTargetView(const PixelFormat format);
		bool CreateDepthStencilView(const PixelFormat format);
		bool CreateShaderResourceView(const PixelFormat format);
		bool CreateUnorderedAccessView(const PixelFormat format);

		static DXGI_FORMAT GetFormat(PixelFormat format);
		static D3D11_USAGE GetUsage(const UsageFlag usage);
		static uint32_t GetAccessFlag(const uint32_t access);
		static uint32_t GetBindFlags(uint32_t flags);

	private:
		DXContext& _context;

		ComPtr<ID3D11Texture2D> _texture;
		ComPtr<ID3D11Texture2D> _stagingTexture;

		ComPtr<ID3D11RenderTargetView> _rtv;
		ComPtr<ID3D11DepthStencilView> _dsv;
		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;

		std::function<void()> _unbindFunc;

		PixelFormat _format;
		UsageFlag _usage;
		uint32_t _acessFlags;

		uint32_t _width;
		uint32_t _height;
	};
}