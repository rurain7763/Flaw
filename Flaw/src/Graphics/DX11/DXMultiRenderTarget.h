#pragma once

#include "Graphics/GraphicsMultiRenderTarget.h"

#include <Windows.h>
#include <wrl.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace DirectX::PackedVector;

namespace flaw {
	class DXContext;
	class DXTexture2D;

	class DXMultiRenderTarget : public GraphicsMultiRenderTarget {
	public:
		DXMultiRenderTarget(DXContext& context, const Descriptor& desc);

		void Bind() override;
		void Resize(int32_t width, int32_t height) override;

		void PushRenderTarget(const GraphicsRenderTarget& renderTarget) override;
		void PopRenderTarget() override;

		void SetBlendMode(int32_t slot, BlendMode blendMode, bool alphaToCoverage) override;

		Ref<Texture2D> GetRenderTargetTex(int32_t slot) override;
		Ref<Texture2D> GetDepthStencilTex() override;

		void ClearAllRenderTargets() override;
		void ClearDepthStencil() override;

	private:
		void CreateBlendState();

	private:
		static constexpr uint32_t MaxRenderTargets = 8;

		DXContext& _context;

		std::vector<GraphicsRenderTarget> _renderTargets;
		GraphicsDepthStencil _depthStencil;

		ComPtr<ID3D11BlendState> _blendState;
	};
}