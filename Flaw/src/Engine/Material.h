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

	enum MaterialTextureType {
		Albedo = 0x1,
		Normal = 0x2,
		Emissive = 0x4,
		Height = 0x8,
		Metallic = 0x10,
		Roughness = 0x20,
		AmbientOcclusion = 0x40,
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
		Ref<Texture2D> metallicTexture;
		Ref<Texture2D> roughnessTexture;
		Ref<Texture2D> ambientOcclusionTexture;

		std::array<Ref<TextureCube>, 4> cubeTextures;
		std::array<Ref<Texture2DArray>, 4> textureArrays;

		int32_t intConstants[4];
		float floatConstants[4];
		vec2 vec2Constants[4];
		vec4 vec4Constants[4];
	};
}
