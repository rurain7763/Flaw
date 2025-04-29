#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics/GraphicsContext.h"
#include "Mesh.h"

#include <map>
#include <vector>

namespace flaw {
	constexpr uint32_t ReservedTextureStartSlot = 50;
	constexpr uint32_t CubeTextureStartSlot = 54;

	enum class GraphicsType {
		DX11
	};

	struct CameraConstants {
		mat4 view = mat4(1.0f);
		mat4 projection = mat4(1.0f);

		vec3 position = vec3(0.0f);
		float padding;
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
			Normal = 0x2,
			Emissive = 0x4,
			Height = 0x8,
		};

		uint32_t reservedTextureBitMask = 0;
		uint32_t cubeTextureBitMask = 0;
		uint32_t paddingMaterialConstants[2];

		int32_t intConstants[4];
		float floatConstants[4];
		vec2 vec2Constants[4];
		vec4 vec4Constants[4];
	};

	struct Camera {
		bool isPerspective;
		vec3 position;
		mat4 view;
		mat4 projection;
		Frustum frustrum;

		bool TestInFrustum(const vec3& boundingSphereCenter, float boundingSphereRadius, const mat4& modelMatrix) const {
			const mat4 mvMatrix = view * modelMatrix;
			
			float maxScale = glm::compMax(vec3(length2(mvMatrix[0]), length2(mvMatrix[1]), length2(mvMatrix[2])));
			maxScale = sqrt(maxScale);

			const float scaledRadius = boundingSphereRadius * maxScale;
			const vec4 transformedBoundingSphereCenter = mvMatrix * vec4(boundingSphereCenter, 1.0);

			for (const auto& plane : frustrum.planes) {
				if (plane.Distance(transformedBoundingSphereCenter) > scaledRadius) {
					return false;
				}
			}

			return true;
		}
	};

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
		BlendMode blendMode = BlendMode::Default;

		Ref<GraphicsShader> shader;

		Ref<Texture2D> albedoTexture;
		Ref<Texture2D> normalTexture;
		Ref<Texture2D> emissiveTexture;
		Ref<Texture2D> heightTexture;

		std::array<Ref<TextureCube>, 4> cubeTextures;

		int32_t intConstants[4];
		float floatConstants[4];
		vec2 vec2Constants[4];
		vec4 vec4Constants[4];
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

	struct Decal {
		mat4 transform;
		mat4 inverseTransform;
		uint32_t textureID;
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

		static Ref<GraphicsRenderPass> GetMainRenderPass();

		static GraphicsCommandQueue& GetCommandQueue();

		static Ref<GraphicsRenderPass> CreateRenderPass(const GraphicsRenderPass::Descriptor& desc);
		static void SetRenderPass(GraphicsRenderPass* renderPass);
		static void ResetRenderPass();

		static void Resize(int32_t width, int32_t height);
		static void GetSize(int32_t& width, int32_t& height);

		static Ref<ComputeShader> CreateComputeShader(const char* filePath);
		static Ref<ComputePipeline> CreateComputePipeline();

		static Ref<GraphicsPipeline> GetMainGraphicsPipeline();
		static Ref<ComputePipeline> GetMainComputePipeline();

		static int32_t Raycast(
			const Ray& ray,
			const std::function<std::vector<BVHNode>& (int32_t)>& getNodesFunc,
			const std::function<std::vector<BVHTriangle>& (int32_t)>& getTrianglesFunc,
			int32_t candidateCount,
			RayHit& hit
		);

		static GraphicsContext& GetGraphicsContext();
	};
}