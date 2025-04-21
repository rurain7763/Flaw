#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics/GraphicsContext.h"

#include <map>
#include <vector>

namespace flaw {
	constexpr uint32_t ReservedTextureStartSlot = 50;
	constexpr uint32_t CubeTextureStartSlot = 54;

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
			Normal = 0x2,
			Emissive = 0x4,
			Height = 0x8,
		};

		uint32_t reservedTextureBitMask = 0;
		uint32_t cubeTextureBitMask = 0;

		int32_t intConstants[4] = { 0 };
		float floatConstants[4] = { 0.0f };
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

	struct PointVertex {
		vec3 position;
	};

	struct Vertex3D {
		vec3 position;
		vec2 texcoord;
		vec3 tangent;
		vec3 normal;
		vec3 binormal;
	};

	struct Camera {
		bool isPerspective;
		mat4 view;
		mat4 projection;
		Frustum frustrum;

		bool TestInFrustum(const std::vector<vec3>& boundingCube, const mat4& modelMatrix) const {
			std::vector<vec3> transformedBoundingCube(boundingCube.size());
			for (int32_t i = 0; i < boundingCube.size(); ++i) {
				transformedBoundingCube[i] = modelMatrix * vec4(boundingCube[i], 1.0);
			}

			for (int i = 0; i < Frustum::PlaneCount; ++i) {
				auto& plane = frustrum.planes[i];
				
				bool isInFrustum = false;
				for (int32_t i = 0; i < boundingCube.size(); ++i) {
					const auto& vertex = vec4(boundingCube[i], 1.0);
					if (plane.Distance(vertex) <= 0.0f) {
						isInFrustum = true;
						break;
					}
				}

				if (!isInFrustum) {
					return false;
				}
			}

			return true;
		}

		bool TestInFrustum(const vec3& boundingSphereCenter, float boundingSphereRadius, const mat4& modelMatrix) const {
			const float maxScale = glm::compMax(vec3(length(modelMatrix[0]), length(modelMatrix[1]), length(modelMatrix[2])));
			const float scaledRadius = boundingSphereRadius * maxScale;
			const vec4 transformedBoundingSphereCenter = modelMatrix * vec4(boundingSphereCenter, 1.0);

			for (const auto& plane : frustrum.planes) {
				if (plane.Distance(transformedBoundingSphereCenter) > scaledRadius) {
					return false;
				}
			}

			return true;
		}
	};

	struct Mesh {
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;

		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;
		
		vec3 boundingSphereCenter = vec3(0.0);
		float boundingSphereRadius = 0;
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

		int32_t intConstants[4] = { 0 };
		float floatConstants[4] = { 0.0f };
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

		static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
		static void GetViewport(int32_t& x, int32_t& y, int32_t& width, int32_t& height);

		static void Resize(int32_t width, int32_t height);
		static void GetSize(int32_t& width, int32_t& height);

		static Ref<ComputeShader> CreateComputeShader(const char* filePath);
		static Ref<ComputePipeline> CreateComputePipeline();

		static GraphicsContext& GetGraphicsContext();
	};
}