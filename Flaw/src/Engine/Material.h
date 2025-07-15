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

	struct MaterialConstants {
		uint32_t reservedTextureBitMask = 0;
		uint32_t cubeTextureBitMask = 0;
		uint32_t textureArrayBitMask = 0;
		uint32_t paddingMaterialConstants;

		int32_t intConstants[4];
		float floatConstants[4];
		vec2 vec2Constants[4];
		vec4 vec4Constants[4];

		vec3 baseColor;
		float padding;
	};

	struct Material {
		RenderMode renderMode = RenderMode::Opaque;

		CullMode cullMode = CullMode::Back;
		DepthTest depthTest = DepthTest::Less;
		bool depthWrite = true;

		Ref<GraphicsShader> shader;

		vec3 baseColor;

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

		inline void FillMaterialConstants(MaterialConstants& outConstants) const {
			outConstants.reservedTextureBitMask = 0;
			if (albedoTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Albedo;
			}
			if (normalTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Normal;
			}
			if (emissiveTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Emissive;
			}
			if (heightTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Height;
			}
			if (metallicTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Metallic;
			}
			if (roughnessTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::Roughness;
			}
			if (ambientOcclusionTexture) {
				outConstants.reservedTextureBitMask |= MaterialTextureType::AmbientOcclusion;
			}
			for (int32_t i = 0; i < cubeTextures.size(); ++i) {
				if (cubeTextures[i]) {
					outConstants.cubeTextureBitMask |= (1 << i);
				}
			}
			for (int32_t i = 0; i < textureArrays.size(); ++i) {
				if (textureArrays[i]) {
					outConstants.textureArrayBitMask |= (1 << i);
				}
			}

			std::memcpy(
				outConstants.intConstants, 
				intConstants, 
				sizeof(uint32_t) * 4 + sizeof(float) * 4 + sizeof(vec2) * 4 + sizeof(vec4) * 4
			);

			outConstants.baseColor = baseColor;
		}
	};
}
