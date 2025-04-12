#pragma once

#include "Graphics/GraphicsType.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <stdexcept>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	static DXGI_FORMAT GetFormat(PixelFormat format) {
		switch (format) {
		case PixelFormat::BGRX8:
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

	static D3D11_USAGE GetUsage(const UsageFlag usage) {
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

	static uint32_t GetAccessFlag(const uint32_t access) {
		uint32_t accessFlag = 0;
		if (access & AccessFlag::Write) {
			accessFlag |= D3D11_CPU_ACCESS_WRITE;
		}
		if (access & AccessFlag::Read) {
			accessFlag |= D3D11_CPU_ACCESS_READ;
		}
		return accessFlag;
	}

	static uint32_t GetBindFlags(uint32_t flags) {
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