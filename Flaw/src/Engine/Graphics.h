#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics/GraphicsContext.h"

namespace flaw {
	constexpr uint32_t DefaultTextureStartSlot = 12;
	constexpr uint32_t CubeTextureStartSlot = 28;

	enum class GraphicsType {
		DX11
	};

	struct VPMatrices {
		mat4 view = mat4(1.0f);
		mat4 projection = mat4(1.0f);
	};

	struct GlobalConstants {
		vec2 screenResolution;
		float time;
		float deltaTime;
	};

	struct LightConstants {
		vec3 ambientColor;
		float ambientIntensity;

		uint32_t numDirectionalLights;
		uint32_t numPointLights;
		uint32_t numSpotLights;
		uint32_t padding;
	};

	struct MaterialConstants {
		enum TextureType {
			Albedo = 0x1,
			Normal = 0x2
		};

		uint32_t defaultTextureBitMask = 0;
		uint32_t cubeTextureBitMask = 0;

		int32_t intConstants[4] = { 0 };
		uint32_t paddingMaterialConstants[2];
	};

	struct QuadVertex {
		vec3 position;
		vec2 texcoord;
		vec4 color;
		uint32_t textureID;
		uint32_t id;
	};

	struct CircleVertex {
		vec3 localPosition;
		vec3 worldPosition;
		float thickness;
		vec4 color;
		uint32_t id;
	};

	struct LineVertex {
		vec3 position;
		vec4 color;
		uint32_t id;
	};

	struct Vertex3D {
		vec3 position;
		vec2 texcoord;
		vec3 tangent;
		vec3 normal;
		vec3 binormal;
	};

	struct Material {
		CullMode cullMode = CullMode::Back;
		DepthTest depthTest = DepthTest::Less;
		bool depthWrite = true;

		Ref<GraphicsShader> shader;

		Ref<Texture2D> albedoTexture;
		Ref<Texture2D> diffuseTexture;
		Ref<Texture2D> normalTexture;

		std::array<Ref<TextureCube>, 4> cubeTextures;

		int32_t intConstants[4] = { 0 };
	};

	struct SkyLight {
		vec3 color = vec3(0.0f);
		float intensity = 0.0f;
	};

	struct DirectionalLight {
		vec3 color = vec3(1.0f);
		vec3 direction = Forward;
		float intensity = 1.0f;
	};

	struct PointLight {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;
		vec3 position = vec3(0.0f);
		float range = 1.0f;
	};

	struct SpotLight {
		vec3 color = vec3(1.0f);
		float intensity = 1.0f;
		vec3 position = vec3(0.0f);
		vec3 direction = Forward;
		float inner;
		float outer;
		float range = 1.0f;
	};

	class Graphics {
	public:
		static void Init(GraphicsType type);
		static void Cleanup();

		static void Prepare();
		static void Present();

		static Ref<VertexBuffer> CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor);
		static Ref<IndexBuffer> CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor);
		static Ref<GraphicsShader> CreateGraphicsShader(const char* filePath, const uint32_t compileFlag);
		static Ref<GraphicsPipeline> CreateGraphicsPipeline();

		static Ref<ConstantBuffer> CreateConstantBuffer(uint32_t size);
		static Ref<StructuredBuffer> CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc);

		static Ref<Texture2D> CreateTexture2D(const Texture2D::Descriptor& descriptor);
		static Ref<TextureCube> CreateTextureCube(const TextureCube::Descriptor& descriptor);

		static void SetRenderTexture(uint32_t slot, Ref<Texture2D> texture, float clearValue[4]);
		static void ResetRenderTexture(uint32_t slot);

		static GraphicsCommandQueue& GetCommandQueue();

		static void CaptureRenderTargetTex(Ref<Texture2D>& dstTexture);

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height);

		static void SetClearColor(float r, float g, float b, float a);

		static void Resize(int32_t width, int32_t height);
		static void GetSize(int32_t& width, int32_t& height);

		static Ref<ComputeShader> CreateComputeShader(const char* filePath);
		static Ref<ComputePipeline> CreateComputePipeline();

		static GraphicsContext& GetGraphicsContext();
	};
}