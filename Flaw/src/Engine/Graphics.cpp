#include "pch.h"
#include "Graphics.h"
#include "Platform.h"
#include "Graphics/GraphicsFunc.h"
#include "Material.h"
#include "Time/Time.h"

#define NOMINMAX
#include "Graphics/DX11/DXContext.h"

namespace flaw {
	struct RaycastUniform {
		vec4 rayOrigin;
		vec4 rayDirection;
		float rayLength;
		int32_t triangleCount;

		int32_t padding[2];
	};

	struct RaycastTriangle {
		vec3 p0;
		vec3 p1;
		vec3 p2;
		vec3 center;
		vec3 normal;
		uint32_t index;
	};

	struct RaycastResult {
		int32_t index;

		vec3 position;
		vec3 normal;
		float distance;
	};

	constexpr int32_t MaxRaycastTriangles = 10000;
	constexpr int32_t MaxRayHits = MaxRaycastTriangles + 1;

	static Scope<GraphicsContext> g_graphicsContext;

	static Ref<GraphicsPipeline> g_graphicsPipeline;
	static Ref<ComputePipeline> g_computePipeline;

	static Ref<ConstantBuffer> g_globalConstantsCB;
	static Ref<ConstantBuffer> g_materialConstantsCB;

	static Ref<StructuredBuffer> g_batchedTransformSB;

	static Ref<ComputeShader> g_raycastShader;
	static Ref<ConstantBuffer> g_raycastUniformCB;
	static Ref<StructuredBuffer> g_raycastTriSB;
	static Ref<StructuredBuffer> g_raycastResultSB;

	void Graphics::Init(GraphicsType type) {
		int32_t width, height;
		Platform::GetFrameBufferSize(width, height);

		switch (type)
		{
		case flaw::GraphicsType::DX11:
			g_graphicsContext = CreateScope<DXContext>(Platform::GetPlatformContext(), width, height);
			break;
		default:
			throw std::runtime_error("Unsupported graphics type");
			break;
		}

		g_graphicsPipeline = CreateGraphicsPipeline();
		g_computePipeline = CreateComputePipeline();

		g_globalConstantsCB = Graphics::CreateConstantBuffer(sizeof(GlobalConstants));
		g_materialConstantsCB = Graphics::CreateConstantBuffer(sizeof(MaterialConstants));

		StructuredBuffer::Descriptor batchedTransformSBDesc = {};
		batchedTransformSBDesc.elmSize = sizeof(mat4);
		batchedTransformSBDesc.count = MaxBatchTransformCount;
		batchedTransformSBDesc.bindFlags = BindFlag::ShaderResource;
		batchedTransformSBDesc.accessFlags = AccessFlag::Write;
		g_batchedTransformSB = Graphics::CreateStructuredBuffer(batchedTransformSBDesc);

		// NOTE: for raycasting
		g_raycastShader = CreateComputeShader("Resources/Shaders/raycast.fx");

		g_raycastUniformCB = CreateConstantBuffer(sizeof(RaycastUniform));

		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(RaycastTriangle);
		sbDesc.count = MaxRaycastTriangles;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		g_raycastTriSB = CreateStructuredBuffer(sbDesc);

		sbDesc.elmSize = sizeof(RaycastResult);
		sbDesc.count = MaxRayHits;
		sbDesc.bindFlags = BindFlag::UnorderedAccess;
		sbDesc.accessFlags = AccessFlag::Read | AccessFlag::Write;
		g_raycastResultSB = CreateStructuredBuffer(sbDesc);

		// TODO: think about this, this is for object picker. but not use in build
		auto mainMrt = Graphics::GetMainRenderPass();

		Texture2D::Descriptor desc = {};
		desc.width = width;
		desc.height = height;
		desc.format = PixelFormat::R32_UINT;
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::RenderTarget;

		GraphicsRenderTarget renderTarget = {};
		renderTarget.blendMode = BlendMode::Disabled;
		renderTarget.texture = g_graphicsContext->CreateTexture2D(desc);
		renderTarget.viewportX = 0;
		renderTarget.viewportY = 0;
		renderTarget.viewportWidth = width;
		renderTarget.viewportHeight = height;
		renderTarget.clearValue = { (float)std::numeric_limits<int32_t>().max(), 0.0f, 0.0f, 0.0f };
		renderTarget.resizeFunc = [](GraphicsRenderTarget& current, int32_t width, int32_t height) {
			Texture2D::Descriptor desc = {};
			desc.width = width;
			desc.height = height;
			desc.format = PixelFormat::R32_UINT;
			desc.usage = UsageFlag::Static;
			desc.bindFlags = BindFlag::RenderTarget;

			current.texture = Graphics::CreateTexture2D(desc);
			current.viewportWidth = width;
			current.viewportHeight = height;
		};

		mainMrt->PushRenderTarget(renderTarget);
	}

	void Graphics::Cleanup() {
		g_raycastResultSB.reset();
		g_raycastTriSB.reset();
		g_raycastUniformCB.reset();
		g_raycastShader.reset();
		g_materialConstantsCB.reset();
		g_globalConstantsCB.reset();
		g_computePipeline.reset();
		g_graphicsPipeline.reset();
		g_graphicsContext.reset();
	}

	void Graphics::Prepare() {
		g_graphicsContext->Prepare();

		GlobalConstants globalConstants = {};

		int32_t width, height;
		Graphics::GetSize(width, height);

		globalConstants.screenResolution = vec2((float)width, (float)height);
		globalConstants.time = Time::GetTime();
		globalConstants.deltaTime = Time::DeltaTime();

		g_globalConstantsCB->Update(&globalConstants, sizeof(GlobalConstants));
	}

	void Graphics::Present() {
		g_graphicsContext->Present();
	}

	Ref<VertexBuffer> Graphics::CreateVertexBuffer(const VertexBuffer::Descriptor& descriptor) {
		return g_graphicsContext->CreateVertexBuffer(descriptor);
	}

	Ref<IndexBuffer> Graphics::CreateIndexBuffer(const IndexBuffer::Descriptor& descriptor) {
		return g_graphicsContext->CreateIndexBuffer(descriptor);
	}

	Ref<GraphicsShader> Graphics::CreateGraphicsShader(const char* filePath, const uint32_t compileFlag) {
		return g_graphicsContext->CreateGraphicsShader(filePath, compileFlag);
	}

	Ref<GraphicsPipeline> Graphics::CreateGraphicsPipeline() {
		return g_graphicsContext->CreateGraphicsPipeline();
	}

	Ref<ConstantBuffer> Graphics::CreateConstantBuffer(uint32_t size) {
		return g_graphicsContext->CreateConstantBuffer(size);
	}

	Ref<StructuredBuffer> Graphics::CreateStructuredBuffer(const StructuredBuffer::Descriptor& desc) {
		return g_graphicsContext->CreateStructuredBuffer(desc);
	}

	Ref<Texture2D> Graphics::CreateTexture2D(const Texture2D::Descriptor& descriptor) {
		return g_graphicsContext->CreateTexture2D(descriptor);
	}

	Ref<Texture2DArray> Graphics::CreateTexture2DArray(const Texture2DArray::Descriptor& descritor) {
		return g_graphicsContext->CreateTexture2DArray(descritor);
	}

	Ref<TextureCube> Graphics::CreateTextureCube(const TextureCube::Descriptor& descriptor) {
		return g_graphicsContext->CreateTextureCube(descriptor);
	}

	Ref<GraphicsRenderPass> Graphics::GetMainRenderPass() {
		return g_graphicsContext->GetMainRenderPass();
	}

	GraphicsCommandQueue& Graphics::GetCommandQueue() {
		return g_graphicsContext->GetCommandQueue();
	}

	Ref<GraphicsRenderPass> Graphics::CreateRenderPass(const GraphicsRenderPass::Descriptor& desc) {
		return g_graphicsContext->CreateRenderPass(desc);
	}

	void Graphics::SetRenderPass(GraphicsRenderPass* renderPass) {
		g_graphicsContext->SetRenderPass(renderPass);
	}

	void Graphics::ResetRenderPass() {
		g_graphicsContext->ResetRenderPass();
	}

	void Graphics::Resize(int32_t width, int32_t height) {
		g_graphicsContext->Resize(width, height);
	}

	void Graphics::GetSize(int32_t& width, int32_t& height) {
		g_graphicsContext->GetSize(width, height);
	}

	Ref<ComputeShader> Graphics::CreateComputeShader(const char* filePath) {
		return g_graphicsContext->CreateComputeShader(filePath);
	}

	Ref<ComputePipeline> Graphics::CreateComputePipeline() {
		return g_graphicsContext->CreateComputePipeline();
	}

	Ref<GraphicsPipeline> Graphics::GetMainGraphicsPipeline() {
		return g_graphicsPipeline;
	}

	Ref<ComputePipeline> Graphics::GetMainComputePipeline() {
		return g_computePipeline;
	}

	int32_t Graphics::Raycast(
		const Ray& ray,
		const std::function<std::vector<BVHNode>& (int32_t)>& getNodesFunc,
		const std::function<std::vector<BVHTriangle>& (int32_t)>& getTrianglesFunc,
		int32_t candidateCount,
		RayHit& hit) 
	{
		std::vector<RaycastTriangle> candidateTris;
		for (uint32_t i = 0; i < candidateCount; i++) {
			auto& nodes = getNodesFunc(i);
			auto& triangles = getTrianglesFunc(i);

			Raycast::GetCandidateBVHTriangles(nodes, triangles, ray, [&](int32_t start, int32_t count) {
				std::transform(triangles.begin() + start, triangles.begin() + start + count, std::back_inserter(candidateTris), [i](const BVHTriangle& tri) {
					return RaycastTriangle{
						tri.p0,
						tri.p1,
						tri.p2,
						tri.center,
						tri.normal,
						i
					};
				});
			});
		}

		if (candidateTris.empty()) {
			return -1;
		}

		g_raycastTriSB->Update(candidateTris.data(), candidateTris.size() * sizeof(RaycastTriangle));

		RaycastUniform raycastUniform = {};
		raycastUniform.rayOrigin = vec4(ray.origin, 1.0f);
		raycastUniform.rayDirection = vec4(ray.direction, 0.0f);
		raycastUniform.rayLength = ray.length;
		raycastUniform.triangleCount = candidateTris.size();
		g_raycastUniformCB->Update(&raycastUniform, sizeof(RaycastUniform));

		// NOTE: first raycast result is represented total result count
		RaycastResult defaultResult = {};
		defaultResult.index = 0;
		g_raycastResultSB->Update(&defaultResult, sizeof(RaycastResult));

		auto& cmdQueue = g_graphicsContext->GetCommandQueue();

		g_computePipeline->SetShader(g_raycastShader);

		cmdQueue.SetComputePipeline(g_computePipeline);
		cmdQueue.SetComputeConstantBuffer(g_raycastUniformCB, 0);
		cmdQueue.SetComputeStructuredBuffer(g_raycastTriSB, BindFlag::ShaderResource, 0);
		cmdQueue.SetComputeStructuredBuffer(g_raycastResultSB, BindFlag::UnorderedAccess, 0);
		cmdQueue.Dispatch(CalculateDispatchGroupCount(1024, candidateTris.size()), 1, 1);

		cmdQueue.Execute();

		std::vector<RaycastResult> results(MaxRayHits);
		g_raycastResultSB->Fetch(results.data(), MaxRayHits * sizeof(RaycastResult));
		
		int32_t resultCount = std::min(results[0].index, MaxRayHits - 1);

		// 후보 중 가장 가까운 것만
		int32_t closestIndex = -1;
		float minDist = std::numeric_limits<float>::max();
		for (int32_t i = 0; i < resultCount; i++) {
			auto& result = results[i + 1];
			
			if (result.distance < minDist) {
				hit.position = result.position;
				hit.normal = result.normal;
				hit.distance = result.distance;

				closestIndex = result.index;
				minDist = result.distance;
			}
		}

		return closestIndex;
	}

	void Graphics::CaptureTexture(const Ref<Texture2D>& srcTex, std::vector<uint8_t>& outData) {
		Texture2D::Descriptor desc = {};
		desc.width = srcTex->GetWidth();
		desc.height = srcTex->GetHeight();
		desc.format = srcTex->GetPixelFormat();
		desc.usage = UsageFlag::Staging;
		desc.access = AccessFlag::Read;
		
		Ref<Texture2D> stagingTexture = g_graphicsContext->CreateTexture2D(desc);
		srcTex->CopyTo(stagingTexture);

		outData.resize(srcTex->GetWidth() * srcTex->GetHeight() * GetSizePerPixel(srcTex->GetPixelFormat()));
		stagingTexture->Fetch(outData.data(), outData.size());
	}

	void Graphics::CaptureTextureArray(const Ref<Texture2DArray>& srcTex, std::vector<uint8_t>& outData) {
		Texture2DArray::Descriptor desc = {};
		desc.fromMemory = true;
		desc.arraySize = srcTex->GetArraySize();
		desc.width = srcTex->GetWidth();
		desc.height = srcTex->GetHeight();
		desc.format = srcTex->GetPixelFormat();
		desc.usage = UsageFlag::Staging;
		desc.access = AccessFlag::Read;

		Ref<Texture2DArray> stagingTexture = g_graphicsContext->CreateTexture2DArray(desc);
		srcTex->CopyTo(stagingTexture);

		outData.resize(srcTex->GetWidth() * srcTex->GetHeight() * GetSizePerPixel(srcTex->GetPixelFormat()) * srcTex->GetArraySize());
		stagingTexture->FetchAll(outData.data());
	}

	Ref<ConstantBuffer> Graphics::GetGlobalConstantsCB() {
		return g_globalConstantsCB;
	}

	Ref<ConstantBuffer> Graphics::GetMaterialConstantsCB() {
		return g_materialConstantsCB;
	}

	Ref<StructuredBuffer> Graphics::GetBatchedTransformSB() {
		return g_batchedTransformSB;
	}
	
	GraphicsContext& Graphics::GetGraphicsContext() {
		return *g_graphicsContext;
	}
}