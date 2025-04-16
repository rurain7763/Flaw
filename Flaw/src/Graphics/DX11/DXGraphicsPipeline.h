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
		void SetCullMode(CullMode cullMode) override;
		void SetFillMode(FillMode fillMode) override;
		void SetBlendMode(BlendMode blendMode, bool alphaToCoverage = false) override;

		void Bind() override;

	private:
		ComPtr<ID3D11RasterizerState> CreateRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode);
		ComPtr<ID3D11DepthStencilState> CreateDepthStencilState(D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask);

		static D3D11_COMPARISON_FUNC GetDepthTest(DepthTest depthTest);

		static D3D11_FILL_MODE GetFillMode(FillMode fillMode);
		static D3D11_CULL_MODE GetCullMode(CullMode cullMode);

	private:
		DXContext& _context;

		DepthTest _depthTest;
		bool _depthWrite;

		ComPtr<ID3D11RasterizerState> _rasterizerState;

		ComPtr<ID3D11DepthStencilState> _depthStencilState;
		std::unordered_map<DepthStencilStateKey, ComPtr<ID3D11DepthStencilState>, DepthStencilStateKeyHash> _depthStencilStateStore;
	};
}