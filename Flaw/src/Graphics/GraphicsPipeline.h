#pragma once

#include "Core.h"
#include "GraphicsBuffers.h"
#include "GraphicsShader.h"

namespace flaw {
	class GraphicsPipeline {
	public:
		GraphicsPipeline() = default;
		virtual ~GraphicsPipeline() = default;

		virtual void SetDepthTest(DepthTest depthTest, bool depthWrite = true) = 0;
		virtual void SetBlendMode(BlendMode blendMode, bool alphaToCoverage = false) = 0;
		virtual void SetCullMode(CullMode cullMode) = 0;
		virtual void SetFillMode(FillMode fillMode) = 0;

		virtual void Bind() = 0;

		inline void SetShader(const Ref<GraphicsShader>& shader) { _shader = shader; }

	protected:
		Ref<GraphicsShader> _shader;

		CullMode _cullMode = CullMode::Back;
		FillMode _fillMode = FillMode::Solid;
	};
}