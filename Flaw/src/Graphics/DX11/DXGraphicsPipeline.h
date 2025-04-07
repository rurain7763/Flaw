#pragma once

#include "Graphics/GraphicsPipeline.h"

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

	class DXGraphicsPipeline : public GraphicsPipeline {
	private:
		struct BlendStateKey {
			D3D11_BLEND srcBlend;
			D3D11_BLEND destBlend;
			bool alphaToCoverage;

			BlendStateKey(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, bool alphaToCoverage)
				: srcBlend(srcBlend)
				, destBlend(destBlend)
				, alphaToCoverage(alphaToCoverage)
			{
			}

			bool operator==(const BlendStateKey& other) const {
				return srcBlend == other.srcBlend && destBlend == other.destBlend && alphaToCoverage == other.alphaToCoverage;
			}
		};

		struct BlendStateKeyHash {
			std::size_t operator()(const BlendStateKey& key) const {
				return std::hash<int>()(key.srcBlend) ^ std::hash<int>()(key.destBlend) ^ std::hash<bool>()(key.alphaToCoverage);
			}
		};

		struct DepthStencilStateKey {
			D3D11_COMPARISON_FUNC depthTest;
			D3D11_DEPTH_WRITE_MASK writeMask;
			DepthStencilStateKey(D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask)
				: depthTest(depthTest)
				, writeMask(writeMask)
			{
			}

			bool operator==(const DepthStencilStateKey& other) const {
				return depthTest == other.depthTest && writeMask == other.writeMask;
			}
		};

		struct DepthStencilStateKeyHash {
			std::size_t operator()(const DepthStencilStateKey& key) const {
				return std::hash<int>()(key.depthTest) ^ std::hash<int>()(key.writeMask);
			}
		};

	public:
		DXGraphicsPipeline(DXContext& context);

		void SetDepthTest(DepthTest depthTest, bool depthWrite = true) override;
		void SetBlendMode(BlendMode blendMode, bool alphaToCoverage = false) override;
		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;

		void Bind() override;

	private:
		ComPtr<ID3D11RasterizerState> CreateRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode);
		ComPtr<ID3D11DepthStencilState> CreateDepthStencilState(D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask);
		ComPtr<ID3D11BlendState> CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, bool alphaToCoverage);

		static D3D11_COMPARISON_FUNC GetDepthTest(DepthTest depthTest);

		static void GetBlend(BlendMode blendMode, D3D11_BLEND& outSrcBlend, D3D11_BLEND& outDestBlend);

		static D3D11_FILL_MODE GetFillMode(FillMode fillMode);
		static D3D11_CULL_MODE GetCullMode(CullMode cullMode);

	private:
		DXContext& _context;

		ComPtr<ID3D11RasterizerState> _rasterizerState;

		ComPtr<ID3D11DepthStencilState> _depthStencilState;
		std::unordered_map<DepthStencilStateKey, ComPtr<ID3D11DepthStencilState>, DepthStencilStateKeyHash> _depthStencilStateStore;

		ComPtr<ID3D11BlendState> _blendState;
		std::unordered_map<BlendStateKey, ComPtr<ID3D11BlendState>, BlendStateKeyHash> _blendStateStore;
	};
}