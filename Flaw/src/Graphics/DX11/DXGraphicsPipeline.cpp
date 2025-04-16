#include "pch.h"
#include "DXGraphicsPipeline.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXGraphicsPipeline::DXGraphicsPipeline(DXContext& context)
		: _context(context)
	{
		_rasterizerState = CreateRasterizerState(D3D11_FILL_SOLID, D3D11_CULL_BACK);

		_depthStencilState = CreateDepthStencilState(D3D11_COMPARISON_LESS, D3D11_DEPTH_WRITE_MASK_ALL);
		if (!_depthStencilState) {
			Log::Error("CreateDepthStencilState failed");
			return;
		}
	}

	void DXGraphicsPipeline::SetDepthTest(DepthTest depthTest, bool depthWrite) {
		if (depthTest == _depthTest && depthWrite == _depthWrite) {
			return;
		}

		_depthTest = depthTest;
		_depthWrite = depthWrite;

		D3D11_COMPARISON_FUNC depthFunc = GetDepthTest(depthTest);
		D3D11_DEPTH_WRITE_MASK writeMask = depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;

		_depthStencilState = CreateDepthStencilState(depthFunc, writeMask);
	}

	void DXGraphicsPipeline::SetCullMode(CullMode cullMode) {
		if (cullMode == _cullMode) {
			return;
		}

		_cullMode = cullMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::SetFillMode(FillMode fillMode) {
		if (fillMode == _fillMode) {
			return;
		}

		_fillMode = fillMode;

		_rasterizerState = CreateRasterizerState(GetFillMode(_fillMode), GetCullMode(_cullMode));
	}

	void DXGraphicsPipeline::SetBlendMode(BlendMode blendMode, bool alphaToCoverage) {
		auto& mainMRT = _context.GetMainMultiRenderTarget();
		mainMRT->SetBlendMode(0, blendMode, alphaToCoverage);
	}

	void DXGraphicsPipeline::Bind() {
		_shader->Bind();
		_context.DeviceContext()->RSSetState(_rasterizerState.Get());
		_context.DeviceContext()->OMSetDepthStencilState(_depthStencilState.Get(), 0);
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
		case DepthTest::Never:
			return D3D11_COMPARISON_NEVER;
		}

		return D3D11_COMPARISON_LESS;
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
}