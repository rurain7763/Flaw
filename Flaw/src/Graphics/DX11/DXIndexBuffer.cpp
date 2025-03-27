#include "pch.h"
#include "DXIndexBuffer.h"

#include "DXContext.h"
#include "Log/Log.h"

namespace flaw {
	DXIndexBuffer::DXIndexBuffer(DXContext& context)
		: _context(context)
		, _indexCount(0)
	{
	}

	DXIndexBuffer::DXIndexBuffer(DXContext& context, const Descriptor& descriptor)
		: _context(context)
		, _indexCount(0)
	{
		_dynamic = descriptor.usage == UsageFlag::Dynamic;
	}

	void DXIndexBuffer::Update(const uint32_t* indices, uint32_t count) {
		if (!_buffer) {
			_buffer = CreateBuffer(_dynamic, indices, count);
		}
		else if (_dynamic) {
			if (_indexCount < count) {
				_buffer.Reset();
				_buffer = CreateBuffer(_dynamic, indices, count);
			}
			else {
				D3D11_MAPPED_SUBRESOURCE mappedResource;
				if (FAILED(_context.DeviceContext()->Map(_buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource))) {
					Log::Error("Failed to map buffer");
				}
				else {
					memcpy(mappedResource.pData, indices, sizeof(uint32_t) * count);
					_context.DeviceContext()->Unmap(_buffer.Get(), 0);
				}
			}
		}
		else {
			_buffer.Reset();
			_buffer = CreateBuffer(_dynamic, indices, count);
		}

		_indexCount = count;
	}

	void DXIndexBuffer::Bind() {
		_context.DeviceContext()->IASetIndexBuffer(_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	}

	ComPtr<ID3D11Buffer> DXIndexBuffer::CreateBuffer(bool dynamic, const uint32_t* indices, uint32_t count) {
		D3D11_BUFFER_DESC bufferDesc = {};
		bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		bufferDesc.ByteWidth = sizeof(uint32_t) * count;

		// set buffer data write dynamically or not 
		bufferDesc.CPUAccessFlags = dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.Usage = dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

		bufferDesc.MiscFlags = 0;

		ComPtr<ID3D11Buffer> buffer;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = indices;

		if (FAILED(_context.Device()->CreateBuffer(&bufferDesc, &initData, buffer.GetAddressOf()))) {
			Log::Error("Buffer Update failed");
		}

		return buffer;
	}
}