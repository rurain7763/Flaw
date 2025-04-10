#include "pch.h"
#include "Renderer.h"
#include "Time/Time.h"

namespace flaw {
	constexpr uint32_t MaxBatchCount = 10000;
	constexpr uint32_t MaxDirectionalLights = 10;
	constexpr uint32_t MaxPointLights = 10;

	struct BatchedData {
		mat4 transform;
		uint32_t id;
	};

	static std::vector<BatchedData> g_cubeBatchDatas;
	static std::vector<BatchedData> g_sphereBatchDatas;

	static Ref<VertexBuffer> g_cubeVB;
	static Ref<IndexBuffer> g_cubeIB;
	static Ref<VertexBuffer> g_sphereVB;
	static Ref<IndexBuffer> g_sphereIB;
	static uint32_t g_sphereIndexCount = 0;

	static Ref<ConstantBuffer> g_vpCB;
	static Ref<ConstantBuffer> g_globalCB;
	static Ref<ConstantBuffer> g_lightCB;
	static Ref<StructuredBuffer> g_batchDataSB;
	static Ref<StructuredBuffer> g_directionalLightSB;
	static Ref<StructuredBuffer> g_pointLightSB;
	static Ref<GraphicsPipeline> g_pipeLine;

	void GenerateCubeMesh(std::vector<Vertex3D>& outVertices, std::vector<uint32_t>& outIndices) {
		outVertices.resize(24);
		
		// front
		outVertices[0].position = vec3(-0.5f, 0.5f, -0.5f);;
		outVertices[0].texcoord = vec2(0.f, 0.f);
		outVertices[0].normal = Backward;

		outVertices[1].position = vec3(0.5f, 0.5f, -0.5f);
		outVertices[1].texcoord = vec2(0.f, 0.f);
		outVertices[1].normal = Backward;

		outVertices[2].position = vec3(0.5f, -0.5f, -0.5f);
		outVertices[2].texcoord = vec2(0.f, 0.f);
		outVertices[2].normal = Backward;

		outVertices[3].position = vec3(-0.5f, -0.5f, -0.5f);
		outVertices[3].texcoord = vec2(0.f, 0.f);
		outVertices[3].normal = Backward;

		// right
		outVertices[4].position = vec3(0.5f, 0.5f, -0.5f);
		outVertices[4].texcoord = vec2(0.f, 0.f);
		outVertices[4].normal = Right;

		outVertices[5].position = vec3(0.5f, 0.5f, 0.5f);
		outVertices[5].texcoord = vec2(0.f, 0.f);
		outVertices[5].normal = Right;

		outVertices[6].position = vec3(0.5f, -0.5f, 0.5f);
		outVertices[6].texcoord = vec2(0.f, 0.f);
		outVertices[6].normal = Right;

		outVertices[7].position = vec3(0.5f, -0.5f, -0.5f);
		outVertices[7].texcoord = vec2(0.f, 0.f);
		outVertices[7].normal = Right;

		// back
		outVertices[8].position = vec3(0.5f, 0.5f, 0.5f);
		outVertices[8].texcoord = vec2(0.f, 0.f);
		outVertices[8].normal = Forward;

		outVertices[9].position = vec3(-0.5f, 0.5f, 0.5f);
		outVertices[9].texcoord = vec2(0.f, 0.f);
		outVertices[9].normal = Forward;

		outVertices[10].position = vec3(-0.5f, -0.5f, 0.5f);
		outVertices[10].texcoord = vec2(0.f, 0.f);
		outVertices[10].normal = Forward;

		outVertices[11].position = vec3(0.5f, -0.5f, 0.5f);
		outVertices[11].texcoord = vec2(0.f, 0.f);
		outVertices[11].normal = Forward;

		// left
		outVertices[12].position = vec3(-0.5f, 0.5f, 0.5f);
		outVertices[12].texcoord = vec2(0.f, 0.f);
		outVertices[12].normal = Left;

		outVertices[13].position = vec3(-0.5f, 0.5f, -0.5f);
		outVertices[13].texcoord = vec2(0.f, 0.f);
		outVertices[13].normal = Left;

		outVertices[14].position = vec3(-0.5f, -0.5f, -0.5f);
		outVertices[14].texcoord = vec2(0.f, 0.f);
		outVertices[14].normal = Left;

		outVertices[15].position = vec3(-0.5f, -0.5f, 0.5f);
		outVertices[15].texcoord = vec2(0.f, 0.f);
		outVertices[15].normal = Left;

		// top
		outVertices[16].position = vec3(-0.5f, 0.5f, 0.5f);
		outVertices[16].texcoord = vec2(0.f, 0.f);
		outVertices[16].normal = Up;

		outVertices[17].position = vec3(0.5f, 0.5f, 0.5f);
		outVertices[17].texcoord = vec2(0.f, 0.f);
		outVertices[17].normal = Up;

		outVertices[18].position = vec3(0.5f, 0.5f, -0.5f);
		outVertices[18].texcoord = vec2(0.f, 0.f);
		outVertices[18].normal = Up;

		outVertices[19].position = vec3(-0.5f, 0.5f, -0.5f);
		outVertices[19].texcoord = vec2(0.f, 0.f);
		outVertices[19].normal = Up;

		// bottom
		outVertices[20].position = vec3(-0.5f, -0.5f, -0.5f);
		outVertices[20].texcoord = vec2(0.f, 0.f);
		outVertices[20].normal = Down;

		outVertices[21].position = vec3(0.5f, -0.5f, -0.5f);
		outVertices[21].texcoord = vec2(0.f, 0.f);
		outVertices[21].normal = Down;

		outVertices[22].position = vec3(0.5f, -0.5f, 0.5f);
		outVertices[22].texcoord = vec2(0.f, 0.f);
		outVertices[22].normal = Down;

		outVertices[23].position = vec3(-0.5f, -0.5f, 0.5f);
		outVertices[23].texcoord = vec2(0.f, 0.f);
		outVertices[23].normal = Down;

		// ÀÎµ¦½º »ý¼º
		outIndices.resize(36);

		for (uint32_t i = 0; i < 6; ++i) {
			uint32_t offset = i * 4;
			outIndices[i * 6 + 0] = offset + 0;
			outIndices[i * 6 + 1] = offset + 1;
			outIndices[i * 6 + 2] = offset + 2;
			outIndices[i * 6 + 3] = offset + 0;
			outIndices[i * 6 + 4] = offset + 2;
			outIndices[i * 6 + 5] = offset + 3;
		}
	}

	void GenerateSphereMesh(std::vector<Vertex3D>& outVertices, std::vector<uint32_t>& outIndices, uint32_t sectorCount, uint32_t stackCount, float radius = 1.0f)
	{
		const float PI = 3.14159265359f;

		for (uint32_t i = 0; i <= stackCount; ++i)
		{
			float stackAngle = PI / 2 - i * (PI / stackCount); // from +pi/2 to -pi/2
			float xy = radius * cosf(stackAngle);
			float z = radius * sinf(stackAngle);

			for (uint32_t j = 0; j <= sectorCount; ++j)
			{
				float sectorAngle = j * (2 * PI / sectorCount); // from 0 to 2pi

				float x = xy * cosf(sectorAngle);
				float y = xy * sinf(sectorAngle);

				vec3 position = { x, y, z };
				vec3 normal = normalize(position);  // Normal calculation in right-handed coordinate
				vec2 uv = {
					(float)j / sectorCount,
					(float)i / stackCount
				};

				outVertices.push_back({ position, uv, vec3(0.f), normal, vec3(0.f)});
			}
		}

		for (uint32_t i = 0; i < stackCount; ++i)
		{
			for (uint32_t j = 0; j < sectorCount; ++j)
			{
				uint32_t first = i * (sectorCount + 1) + j;
				uint32_t second = first + sectorCount + 1;

				// To switch to clockwise ordering, we reverse the order of triangle vertices
				outIndices.push_back(first);    // First triangle: first, second + 1, first + 1
				outIndices.push_back(second + 1);
				outIndices.push_back(first + 1);

				outIndices.push_back(first);    // Second triangle: first, second, second + 1
				outIndices.push_back(second);
				outIndices.push_back(second + 1);
			}
		}
	}

	void Renderer::Init() {
		auto& context = Graphics::GetGraphicsContext();

		g_cubeBatchDatas.resize(MaxBatchCount);
		g_sphereBatchDatas.resize(MaxBatchCount);

		// cube
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;
		GenerateCubeMesh(vertices, indices);

		VertexBuffer::Descriptor vbDesc = {};
		vbDesc.usage = UsageFlag::Static;
		vbDesc.elmSize = sizeof(Vertex3D);
		vbDesc.bufferSize = sizeof(Vertex3D) * vertices.size();
		vbDesc.initialData = vertices.data();

		g_cubeVB = context.CreateVertexBuffer(vbDesc);

		IndexBuffer::Descriptor ibDesc = {};
		ibDesc.usage = UsageFlag::Static;
		ibDesc.bufferSize = sizeof(uint32_t) * indices.size();
		ibDesc.initialData = indices.data();

		g_cubeIB = context.CreateIndexBuffer(ibDesc);

		// sphere
		std::vector<Vertex3D> sphereVertices;
		std::vector<uint32_t> sphereIndices;
		GenerateSphereMesh(sphereVertices, sphereIndices, 20, 20);

		VertexBuffer::Descriptor sphereVBDesc = {};
		sphereVBDesc.usage = UsageFlag::Static;
		sphereVBDesc.elmSize = sizeof(Vertex3D);
		sphereVBDesc.bufferSize = sizeof(Vertex3D) * sphereVertices.size();
		sphereVBDesc.initialData = sphereVertices.data();

		g_sphereVB = context.CreateVertexBuffer(sphereVBDesc);

		IndexBuffer::Descriptor sphereIBDesc = {};
		sphereIBDesc.usage = UsageFlag::Static;
		sphereIBDesc.bufferSize = sizeof(uint32_t) * sphereIndices.size();
		sphereIBDesc.initialData = sphereIndices.data();
		g_sphereIB = context.CreateIndexBuffer(sphereIBDesc);
		g_sphereIndexCount = (uint32_t)sphereIndices.size();

		// create constant buffers
		g_vpCB = context.CreateConstantBuffer(sizeof(VPMatrices));
		g_globalCB = context.CreateConstantBuffer(sizeof(GlobalConstants));
		g_lightCB = context.CreateConstantBuffer(sizeof(LightConstants));

		// Create a structured buffer for transforms
		StructuredBuffer::Descriptor sbDesc = {};
		sbDesc.elmSize = sizeof(BatchedData);
		sbDesc.count = MaxBatchCount;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		sbDesc.accessFlags = AccessFlag::Write;
		g_batchDataSB = context.CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for directional lights
		sbDesc.elmSize = sizeof(DirectionalLight);
		sbDesc.count = MaxDirectionalLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		g_directionalLightSB = context.CreateStructuredBuffer(sbDesc);

		// Create a structured buffer for point lights
		sbDesc.elmSize = sizeof(PointLight);
		sbDesc.count = MaxPointLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		g_pointLightSB = context.CreateStructuredBuffer(sbDesc);

		// Create default pipeline
		Ref<GraphicsShader> shader = context.CreateGraphicsShader("Resources/Shaders/std3d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		shader->AddInputElement<float>("POSITION", 3);
		shader->AddInputElement<float>("TEXCOORD", 2);
		shader->AddInputElement<float>("TANGENT", 3);
		shader->AddInputElement<float>("NORMAL", 3);
		shader->AddInputElement<float>("BINORMAL", 3);
		shader->CreateInputLayout();

		g_pipeLine = context.CreateGraphicsPipeline();
		g_pipeLine->SetShader(shader);
		g_pipeLine->SetBlendMode(BlendMode::Default);
		g_pipeLine->SetCullMode(CullMode::Back);
	}

	void Renderer::Cleanup() {
		g_cubeIB.reset();
		g_cubeVB.reset();
		g_sphereIB.reset();
		g_sphereVB.reset();
		g_vpCB.reset();
		g_lightCB.reset();
		g_batchDataSB.reset();
		g_directionalLightSB.reset();
		g_pointLightSB.reset();
		g_globalCB.reset();
		g_pipeLine.reset();
	}

	void Renderer::Begin(const RenderEnvironment& env) {
		VPMatrices vp;
		vp.view = env.view;
		vp.projection = env.projection;
		g_vpCB->Update(&vp, sizeof(VPMatrices));

		GlobalConstants globalConstants;
		int32_t width, height;
		Graphics::GetSize(width, height);
		globalConstants.screenResolution = vec2((float)width, (float)height);
		globalConstants.time = Time::GetTime();
		globalConstants.deltaTime = Time::DeltaTime();
		g_globalCB->Update(&globalConstants, sizeof(GlobalConstants));

		LightConstants lightConstants;
		lightConstants.ambientColor = env.skyLight.color;
		lightConstants.ambientIntensity = env.skyLight.intensity;
		lightConstants.numDirectionalLights = std::min((uint32_t)env.directionalLights.size(), MaxDirectionalLights);
		lightConstants.numPointLights = std::min((uint32_t)env.pointLights.size(), MaxPointLights);
		g_lightCB->Update(&lightConstants, sizeof(LightConstants));

		// Update the structured buffer with directional lights
		g_directionalLightSB->Update(env.directionalLights.data(), sizeof(DirectionalLight) * lightConstants.numDirectionalLights);
	
		// Update the structured buffer with point lights
		g_pointLightSB->Update(env.pointLights.data(), sizeof(PointLight) * lightConstants.numPointLights);

		g_cubeBatchDatas.clear();
		g_sphereBatchDatas.clear();
	}

	void Renderer::End() {
		auto& cmdQueue = Graphics::GetCommandQueue();

		g_batchDataSB->Update(g_cubeBatchDatas.data(), sizeof(BatchedData) * g_cubeBatchDatas.size());

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetGraphicsPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_cubeVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.DrawIndexedInstanced(g_cubeIB, 36, g_cubeBatchDatas.size());
		cmdQueue.End();

		cmdQueue.Execute();

		g_batchDataSB->Update(g_sphereBatchDatas.data(), sizeof(BatchedData) * g_sphereBatchDatas.size());

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetGraphicsPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_sphereVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.DrawIndexedInstanced(g_sphereIB, g_sphereIndexCount, g_sphereBatchDatas.size());
		cmdQueue.End();

		cmdQueue.Execute();
	}

	void Renderer::DrawCube(const uint32_t id, const mat4& transform) {
		BatchedData data;
		data.transform = transform;
		data.id = id;
		g_sphereBatchDatas.push_back(std::move(data));
	}

	void Renderer::DrawSphere(const uint32_t id, const mat4& transform) {
		BatchedData data;
		data.transform = transform;
		data.id = id;
		g_sphereBatchDatas.push_back(std::move(data));
	}
}