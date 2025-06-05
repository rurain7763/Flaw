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
		Ref<TextureCube> prefilteredTexture;
		Ref<Texture2D> brdfLUTTexture; // brdf lookup texture
	};

	class SkyBoxSystem {
	public:
		SkyBoxSystem(Scene& scene);

		void Update();
		void Render(Ref<ConstantBuffer> vpCB);

		SkyBox& GetSkyBox() { return _skybox; }

	private:
		Ref<TextureCube> GenerateCubemap(Ref<Texture2D> texture);
		Ref<TextureCube> GenerateIrradianceCubemap(Ref<TextureCube> cubemap);
		Ref<TextureCube> GeneratePrefilteredCubemap(Ref<TextureCube> cubemap);

		Ref<Texture2D> GenerateBRDFLUTTexture();

	private:
		static constexpr uint32_t CubeTextureSize = 1024; // Cubemap size
		static constexpr uint32_t CubeMipLevels = 5; // Number of mip levels for cubemap
		static constexpr uint32_t IrradianceTextureSize = 32; // Irradiance cubemap size
		static constexpr uint32_t PrefilteredTextureSize = 128; // Prefiltered cubemap size
		static constexpr uint32_t PrefilteredMipLevels = 5; // Number of mip levels for prefiltered cubemap
		static constexpr uint32_t BRDFLUTTextureSize = 512; // Size of BRDF LUT texture

		Scene& _scene;

		Ref<GraphicsShader> _cubeMapShader;
		Ref<GraphicsShader> _irradianceShader;
		Ref<GraphicsShader> _prefilteredShader;
		Ref<GraphicsShader> _brdfLUTShader;
		Ref<GraphicsShader> _skyBoxShader;

		SkyBox _skybox;
	};
}