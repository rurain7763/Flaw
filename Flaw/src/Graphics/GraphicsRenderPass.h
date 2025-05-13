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

		Ref<Texture2D> texture;
		std::array<float, 4> clearValue;
		std::function<Ref<Texture2D>(Ref<Texture2D>& current, int32_t width, int32_t height)> resizeFunc;
		std::function<void(float& x, float& y, float& width, float& height)> viewportFunc;
	};

	struct GraphicsDepthStencil {
		Ref<Texture2D> texture;
		std::function<Ref<Texture2D>(Ref<Texture2D>& current, int32_t width, int32_t height)> resizeFunc;
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

		virtual Ref<Texture2D> GetRenderTargetTex(int32_t slot) = 0;
		virtual Ref<Texture2D> GetDepthStencilTex() = 0;

		virtual void ClearAllRenderTargets() = 0;
		virtual void ClearDepthStencil() = 0;
	};
}