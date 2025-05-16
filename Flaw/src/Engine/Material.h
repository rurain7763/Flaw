#pragma once

#include "Core.h"
#include "Graphics.h"

namespace flaw {
	enum class RenderMode {
		Opaque,
		Masked,
		Transparent,
		Count
	};

	struct Material {
		RenderMode renderMode = RenderMode::Opaque;

		CullMode cullMode = CullMode::Back;
		DepthTest depthTest = DepthTest::Less;
		bool depthWrite = true;

		Ref<GraphicsShader> shader;

		Ref<Texture2D> albedoTexture;
		Ref<Texture2D> normalTexture;
		Ref<Texture2D> emissiveTexture;
		Ref<Texture2D> heightTexture;

		std::array<Ref<TextureCube>, 4> cubeTextures;
		std::array<Ref<Texture2DArray>, 4> textureArrays;

		int32_t intConstants[4];
		float floatConstants[4];
		vec2 vec2Constants[4];
		vec4 vec4Constants[4];
	};
}
