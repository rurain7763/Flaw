#pragma once

#include "Graphics/GraphicsBuffers.h"

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

	class DXStructuredBuffer : public StructuredBuffer {
	public:
		DXStructuredBuffer(DXContext& context);
		DXStructuredBuffer(DXContext& context, const Descriptor& desc);

		virtual ~DXStructuredBuffer() override = default;

		void Create(const Descriptor& desc) override;

		void Update(const void* data, uint32_t size) override;
		void Fetch(void* data, uint32_t size) override;

		void BindToGraphicsShader(const uint32_t slot) override;
		void BindToComputeShader(const BindFlag bindFlag, const uint32_t slot) override;

		void Unbind() override;

		uint32_t Size() const override { return _size; }

	private:
		ComPtr<ID3D11Buffer> CreateBuffer(uint32_t elmSize, uint32_t count, uint32_t bindFlag, AccessFlag usage, const void* initData);
		ComPtr<ID3D11ShaderResourceView> CreateShaderResourceView(uint32_t count);
		ComPtr<ID3D11UnorderedAccessView> CreateUnorderedAccessView(uint32_t count);

		static uint32_t GetBindFlag(uint32_t bindFlag);

	private:
		DXContext& _context;

		ComPtr<ID3D11Buffer> _buffer;
		ComPtr<ID3D11Buffer> _writeOnlyBuffer;
		ComPtr<ID3D11Buffer> _readOnlyBuffer;

		ComPtr<ID3D11ShaderResourceView> _srv;
		ComPtr<ID3D11UnorderedAccessView> _uav;

		std::function<void()> _unbindFunc;

		uint32_t _size = 0;
	};
}