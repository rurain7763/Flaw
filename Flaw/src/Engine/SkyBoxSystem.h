#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"
#include "RenderQueue.h"

namespace flaw {
	class Scene;

	class SkyBoxSystem {
	public:
		SkyBoxSystem(Scene& scene);

		void Update();
		void Render(
			Ref<ConstantBuffer> vpCB,
			Ref<ConstantBuffer> globalCB,
			Ref<ConstantBuffer> lightCB,
			Ref<ConstantBuffer> materialCB
		);

	private:
		Scene& _scene;

		Ref<GraphicsShader> _skyBoxShader;
		Ref<Texture2D> _skyBoxTexture2D;
		Ref<TextureCube> _skyBoxTextureCube;

		Ref<Material> _skyBoxMaterial;
	};
}