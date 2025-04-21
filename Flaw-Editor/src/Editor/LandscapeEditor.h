#pragma once

#include <Flaw.h>

namespace flaw {
	class LandscapeEditor {
	public:
		LandscapeEditor(Application& app);

		void OnRender();

	private:
		void UpdateLandscapeTexture();

	private:
		Application& _app;

		struct LandscapeUniform {
			uint32_t width;
			uint32_t height;
		};

		Ref<ConstantBuffer> _landscapeUniformCB;
		Ref<Texture2D> _landscapeTexture;

		Ref<ComputeShader> _landscapeShader;
		Ref<ComputePipeline> _landscapePipeline;
	};
}