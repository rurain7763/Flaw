#pragma once

#if false
#include "Core.h"

#include "Math.h"
#include "Serialization.h"

namespace flaw {
	struct TexturedVertex {
		vec3 position;
		vec4 color;
		vec2 texCoord;

		void Serialize(Archive& archive) const {
			archive << position.x << position.y << position.z;
			archive << color.x << color.y << color.z << color.w;
			archive << texCoord.x << texCoord.y;
		}

		void Deserialize(Archive& archive) {
			archive >> position.x >> position.y >> position.z;
			archive >> color.x >> color.y >> color.z >> color.w;
			archive >> texCoord.x >> texCoord.y;
		}
	};

	struct ColoredVertex {
		vec3 position;
		vec4 color;
	};

	struct BasicVertex {
		vec3 position;
	};

	struct TexcoordVertex {
		vec3 position;
		vec2 texCoord;
	};

	template<typename T>
	struct MeshData {
		std::vector<T> vertices;
		std::vector<uint32_t> indices;
	};

	struct MVPMatrices {
		mat4 model;
		mat4 view;
		mat4 projection;
	};

	struct ConstUserData {
		int32_t iAttr[4];
		float fAttr[4];
		vec2 v2Attr[4];
		vec4 v4Attr[4];
		mat4 mAttr[4];
	};

	struct Animation2DData {
		int32_t valid;
		int32_t padding[2];
		float sizeFactor;
		vec4 offsetRect;
	};

	struct SkyLightData {
		vec4 color;
		float intensity;

	private:
		int32_t padding[3];
	};

	struct ShaderGlobalData {
		ivec2 resolution;
		float time;
		float deltaTime;

		int32_t pointLight2DCount;
		int32_t directionalLight2DCount;
		int32_t spotLight2DCount;

	private:
		int32_t padding;
	};

	struct TextureBindingData {
		int32_t slots[16] = {
			-1, -1, -1, -1, -1, // texture slot
			-1, -1, -1, -1, // texture cube slot
			-1, -1, -1, -1, // texture array slot
			-1, -1, -1
		};
	};

	struct PointLight2DData {
		vec4 position;
		vec4 color;
		float intensity;
		float radius;

	private:
		int32_t padding[2];
	};

	struct DirectionalLight2DData {
		vec4 direction;
		vec4 color;
		float intensity;

	private:
		int32_t padding[3];
	};

	struct SpotLight2DData {
		vec4 position;
		vec4 direction;
		vec4 color;
		float intensity;
		float radius;
		float angle;

	private:
		int32_t padding;
	};	
}
#endif
