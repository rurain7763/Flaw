#pragma once

#include "Core.h"
#include "Math/Math.h"
#include "Graphics/GraphicsContext.h"
#include "Utils/Raycast.h"
#include "Utils/SerializationArchive.h"

#include <map>
#include <vector>

namespace flaw {
	constexpr uint32_t ReservedTextureStartSlot = 50;
	constexpr uint32_t CubeTextureStartSlot = 57;
	constexpr uint32_t TextureArrayStartSlot = 61;
	
	constexpr static uint32_t MaxBatchedDataCount = 10000;

	enum class GraphicsType {
		DX11
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
	};

	struct SkyLight {
		vec3 color = vec3(0.0f);
		float intensity = 0.0f;
	};

	struct Decal {
		mat4 transform;
		mat4 inverseTransform;
		uint32_t textureID;
	};

	struct BatchedData {
		mat4 worldMatrix;
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
		static Ref<Texture2DArray> CreateTexture2DArray(const Texture2DArray::Descriptor& descriptor);
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

		static void CaptureTexture(const Ref<Texture2D>& srcTex, std::vector<uint8_t>& outData);
		static void CaptureTextureArray(const Ref<Texture2DArray>& srcTex, std::vector<uint8_t>& outData);

		static Ref<ConstantBuffer> GetGlobalConstantsCB();
		static Ref<ConstantBuffer> GetMaterialConstantsCB();
		
		static Ref<StructuredBuffer> GetBatchedDataSB();

		static GraphicsContext& GetGraphicsContext();
	};
}