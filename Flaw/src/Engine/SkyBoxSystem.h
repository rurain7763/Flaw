#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"
#include "RenderQueue.h"

namespace flaw {
	class Scene;

	struct SkyBox {
		Ref<Texture> originalTexture;

		Ref<TextureCube> cubemapTexture;
		Ref<TextureCube> irradianceTexture;
	};

	class SkyBoxSystem {
	public:
		SkyBoxSystem(Scene& scene);

		void Update();
		void Render(Ref<ConstantBuffer> vpCB);

	private:
		Ref<TextureCube> MakeCubemapFromTexture2D(Ref<Texture2D> texture);
		Ref<TextureCube> MakeIrradianceCubemap(Ref<TextureCube> cubemap);

	private:
		static constexpr uint32_t CubeTextureSize = 1024; // Cubemap size

		Scene& _scene;

		Ref<GraphicsShader> _cubeMapShader;
		Ref<GraphicsShader> _skyBoxShader;

		SkyBox _skybox;
	};
}