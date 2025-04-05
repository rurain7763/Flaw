#pragma once

#include "Core.h"
#include "Graphics/GraphicsCommandQueue.h"

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

	class DXCommandQueue : public GraphicsCommandQueue {
	public:
		DXCommandQueue(DXContext& context);
		virtual ~DXCommandQueue() = default;

		void SetPipeline(const Ref<GraphicsPipeline>& pipeline) override;
		void SetVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		void SetConstantBuffer(const Ref<ConstantBuffer>& constantBuffer, uint32_t slot) override;
		void SetTexture(const Ref<Texture2D>& texture, uint32_t slot) override;
		void DrawIndexed(const Ref<IndexBuffer>& indexBuffer, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;
		void DrawIndexedInstanced(const Ref<IndexBuffer>& indexBuffer, uint32_t instanceCount, uint32_t indexOffset = 0, uint32_t vertexOffset = 0) override;

		void Begin() override;
		void End() override;
		void Execute() override;

	private:
		DXContext& _context;

		bool _open = false;
		std::queue<std::function<void()>> _commands;
	};
}