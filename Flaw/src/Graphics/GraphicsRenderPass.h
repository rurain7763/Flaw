#pragma once

#include "Core.h"
#include "Graphics/Texture.h"

#include <vector>
#include <array>
#include <functional>

namespace flaw {
	struct GraphicsRenderTarget {
		BlendMode blendMode = BlendMode::Default;
		bool alphaToCoverage = false;

		Ref<Texture> texture;
		float viewportX, viewportY, viewportWidth, viewportHeight;
		uint32_t mipLevel = 0;
		std::array<float, 4> clearValue;

		std::function<void(GraphicsRenderTarget& current, int32_t width, int32_t height)> resizeFunc;
	};

	struct GraphicsDepthStencil {
		Ref<Texture> texture;
		uint32_t mipLevel = 0;

		std::function<void(GraphicsDepthStencil& current, int32_t width, int32_t height)> resizeFunc;
	};

	class GraphicsRenderPass {
	public:
		struct Descriptor {
			std::vector<GraphicsRenderTarget> renderTargets;
			GraphicsDepthStencil depthStencil;
		};

		virtual ~GraphicsRenderPass() = default;

		virtual void Bind(bool clearColor = true, bool clearDepthStencil = true) = 0;
		virtual void Unbind() = 0;

		virtual void Resize(int32_t width, int32_t height) = 0;

		virtual void PushRenderTarget(const GraphicsRenderTarget& renderTarget) = 0;
		virtual void PopRenderTarget() = 0;

		virtual void SetBlendMode(int32_t slot, BlendMode blendMode, bool alphaToCoverage) = 0;
		virtual void SetViewport(int32_t slot, float x, float y, float width, float height) = 0;

		virtual void SetRenderTargetMipLevel(int32_t slot, uint32_t mipLevel) = 0;
		virtual void SetDepthStencilMipLevel(uint32_t mipLevel) = 0;

		virtual Ref<Texture> GetRenderTargetTex(int32_t slot) = 0;
		virtual Ref<Texture> GetDepthStencilTex() = 0;

		virtual void ClearAllRenderTargets() = 0;
		virtual void ClearDepthStencil() = 0;

		virtual uint32_t GetRenderTargetCount() const = 0;
	};
}