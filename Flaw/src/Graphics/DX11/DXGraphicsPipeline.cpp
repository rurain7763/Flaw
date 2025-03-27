#include "pch.h"
#include "DXGraphicsPipeline.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsPipeline::DXGraphicsPipeline(DXContext& context)
		: _context(context)
	{
		SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		_rasterizerState = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK);

		_depthStencilState = CreateDepthStencilState(D3D11_COMPARISON_LESS, D3D11_DEPTH_WRITE_MASK_ALL);
		if (!_depthStencilState) {
			Log::Error("CreateDepthStencilState failed");
			return;
		}

		_blendState = CreateBlendState(D3D11_BLEND_ONE, D3D11_BLEND_ZERO, false);
		if (!_blendState) {
			Log::Error("CreateBlendState failed");
			return;
		}
	}

	void DXGraphicsPipeline::SetPrimitiveTopology(PrimitiveTopology primitiveTopology) {
		switch (primitiveTopology)
		{
		case PrimitiveTopology::TriangleList:
			_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		case PrimitiveTopology::TriangleStrip:
			_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
			break;
		case PrimitiveTopology::LineList:
			_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case PrimitiveTopology::LineStrip:
			_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
			break;
		case PrimitiveTopology::PointList:
			_primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
			break;
		}
	}

	void DXGraphicsPipeline::SetDepthTest(DepthTest depthTest, bool depthWrite) {
		D3D11_COMPARISON_FUNC depthFunc = GetDepthTest(depthTest);
		D3D11_DEPTH_WRITE_MASK writeMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

		_depthStencilState = CreateDepthStencilState(depthFunc, writeMask);
	}

	void DXGraphicsPipeline::SetBlendMode(BlendMode blendMode, bool alphaToCoverage) {
		D3D11_BLEND srcBlend, destBlend;
		GetBlend(blendMode, srcBlend, destBlend);

		_blendState = CreateBlendState(srcBlend, destBlend, alphaToCoverage);
	}

	void DXGraphicsPipeline::SetCullMode(CullMode cullMode) {
		_cullMode = cullMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::SetFillMode(FillMode fillMode) {
		_fillMode = fillMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::Bind() {
		_shader->Bind();
		_context.DeviceContext()->RSSetState(_rasterizerState.Get());
		_context.DeviceContext()->IASetPrimitiveTopology(_primitiveTopology);
		_context.DeviceContext()->OMSetDepthStencilState(_depthStencilState.Get(), 0);
		_context.DeviceContext()->OMSetBlendState(_blendState.Get(), nullptr, 0xffffffff);
	}

	D3D11_COMPARISON_FUNC DXGraphicsPipeline::GetDepthTest(DepthTest depthTest) {
		switch (depthTest)
		{
		case DepthTest::Less:
			return D3D11_COMPARISON_LESS;
		case DepthTest::LessEqual:
			return D3D11_COMPARISON_LESS_EQUAL;
		case DepthTest::Greater:
			return D3D11_COMPARISON_GREATER;
		case DepthTest::GreaterEqual:
			return D3D11_COMPARISON_GREATER_EQUAL;
		case DepthTest::Equal:
			return D3D11_COMPARISON_EQUAL;
		case DepthTest::NotEqual:
			return D3D11_COMPARISON_NOT_EQUAL;
		case DepthTest::Always:
			return D3D11_COMPARISON_ALWAYS;
		}

		return D3D11_COMPARISON_LESS;
	}

	void DXGraphicsPipeline::GetBlend(BlendMode blendMode, D3D11_BLEND& outSrcBlend, D3D11_BLEND& outDestBlend) {
		switch (blendMode)
		{
		case BlendMode::Default:
			outSrcBlend = D3D11_BLEND_ONE;
			outDestBlend = D3D11_BLEND_ZERO;
			break;
		case BlendMode::Alpha:
			outSrcBlend = D3D11_BLEND_SRC_ALPHA;
			outDestBlend = D3D11_BLEND_INV_SRC_ALPHA;
			break;
		case BlendMode::Additive:
			outSrcBlend = D3D11_BLEND_SRC_ALPHA;
			outDestBlend = D3D11_BLEND_ONE;
			break;
		default:
			outSrcBlend = D3D11_BLEND_ONE;
			outDestBlend = D3D11_BLEND_ZERO;
			break;
		}
	}

	D3D11_FILL_MODE DXGraphicsPipeline::GetFillMode(FillMode fillMode) {
		switch (fillMode)
		{
		case FillMode::Solid:
			return D3D11_FILL_SOLID;
		case FillMode::Wireframe:
			return D3D11_FILL_WIREFRAME;
		}

		return D3D11_FILL_SOLID;
	}

	D3D11_CULL_MODE DXGraphicsPipeline::GetCullMode(CullMode cullMode) {
		switch (cullMode)
		{
		case CullMode::None:
			return D3D11_CULL_NONE;
		case CullMode::Front:
			return D3D11_CULL_FRONT;
		case CullMode::Back:
			return D3D11_CULL_BACK;
		}

		return D3D11_CULL_BACK;
	}

	ComPtr<ID3D11RasterizerState> DXGraphicsPipeline::CreateRasterizerState(D3D11_FILL_MODE fillMode, D3D11_CULL_MODE cullMode) {
		D3D11_RASTERIZER_DESC rasterizerDesc = {};
		rasterizerDesc.FillMode = fillMode;
		rasterizerDesc.CullMode = cullMode;

		ComPtr<ID3D11RasterizerState> rasterizerState;
		if (FAILED(_context.Device()->CreateRasterizerState(&rasterizerDesc, &rasterizerState))) {
			return nullptr;
		}

		return rasterizerState;
	}

	ComPtr<ID3D11DepthStencilState> DXGraphicsPipeline::CreateDepthStencilState(D3D11_COMPARISON_FUNC depthTest, D3D11_DEPTH_WRITE_MASK writeMask) {
		DepthStencilStateKey key(depthTest, writeMask);
		auto it = _depthStencilStateStore.find(key);
		if (it != _depthStencilStateStore.end()) {
			return it->second;
		}
		
		D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
		depthStencilDesc.DepthEnable = TRUE;
		depthStencilDesc.DepthFunc = depthTest;
		depthStencilDesc.DepthWriteMask = writeMask;
		depthStencilDesc.StencilEnable = FALSE;

		ComPtr<ID3D11DepthStencilState> depthStencilState;
		if (FAILED(_context.Device()->CreateDepthStencilState(&depthStencilDesc, &depthStencilState))) {
			return nullptr;
		}

		_depthStencilStateStore.emplace(key, depthStencilState);

		return depthStencilState;
	}

	ComPtr<ID3D11BlendState> DXGraphicsPipeline::CreateBlendState(D3D11_BLEND srcBlend, D3D11_BLEND destBlend, bool alphaToCoverage) {
		BlendStateKey key(srcBlend, destBlend, alphaToCoverage);
		auto it = _blendStateStore.find(key);
		if (it != _blendStateStore.end()) {
			return it->second;
		}
		
		D3D11_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = alphaToCoverage;
		blendDesc.IndependentBlendEnable = TRUE; // blend only applies to the first render target, so other render targets blend settings are ignored
		blendDesc.RenderTarget[0].BlendEnable = TRUE;
		blendDesc.RenderTarget[0].SrcBlend = srcBlend;
		blendDesc.RenderTarget[0].DestBlend = destBlend;
		blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
		blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		// set default values for other render targets
		for (int32_t i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++) {
			blendDesc.RenderTarget[i].BlendEnable = FALSE;
			blendDesc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		}

		ComPtr<ID3D11BlendState> blendState;
		if (FAILED(_context.Device()->CreateBlendState(&blendDesc, &blendState))) {
			return nullptr;
		}

		_blendStateStore.emplace(key, blendState);

		return blendState;
	}
}