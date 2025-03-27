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

	class DXIndexBuffer : public IndexBuffer {
	public:
		DXIndexBuffer(DXContext& context);
		DXIndexBuffer(DXContext& context, const Descriptor& descriptor);
		~DXIndexBuffer() override = default;

		void Update(const uint32_t* indices, uint32_t count) override;
		void Bind() override;

		uint32_t IndexCount() const override { return _indexCount; }

	private:
		ComPtr<ID3D11Buffer> CreateBuffer(bool dynamic, const uint32_t* indices, uint32_t count);

	private:
		DXContext& _context;
		ComPtr<ID3D11Buffer> _buffer;
		uint32_t _indexCount;

		bool _dynamic;
	};
}
