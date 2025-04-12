#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics.h"

namespace flaw {
	class Scene;

	class SkyBoxSystem {
	public:
		SkyBoxSystem(Scene& scene);

		void Update();
		void Render(const mat4& view, const mat4& proj);

	private:
		Scene& _scene;

		Ref<GraphicsShader> _skyBoxShader;
		Ref<Texture2D> _skyBoxTexture2D;
		Ref<TextureCube> _skyBoxTextureCube;
	};
}