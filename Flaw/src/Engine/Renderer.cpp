#include "pch.h"
#include "Renderer.h"
#include "Time/Time.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
	constexpr uint32_t MaxBatchCount = 10000;
	constexpr uint32_t MaxDirectionalLights = 10;
	constexpr uint32_t MaxPointLights = 10;
	constexpr uint32_t MaxSpotLights = 10;

	struct BatchedData {
		mat4 transform;
		uint32_t id;
	};

	static std::vector<BatchedData> g_cubeBatchDatas;
	static std::vector<BatchedData> g_sphereBatchDatas;

	static Ref<GraphicsShader> g_std3dShader;

	static Ref<VertexBuffer> g_cubeVB;
	static Ref<IndexBuffer> g_cubeIB;
	static Ref<VertexBuffer> g_sphereVB;
	static Ref<IndexBuffer> g_sphereIB;

	static Ref<ConstantBuffer> g_vpCB;
	static Ref<ConstantBuffer> g_globalCB;
	static Ref<ConstantBuffer> g_lightCB;
	static Ref<ConstantBuffer> g_materialCB;
	static Ref<StructuredBuffer> g_batchDataSB;
	static Ref<StructuredBuffer> g_directionalLightSB;
	static Ref<StructuredBuffer> g_pointLightSB;
	static Ref<StructuredBuffer> g_spotLightSB;
	static Ref<GraphicsPipeline> g_pipeLine;
	
	void Renderer::Init() {
		auto& context = Graphics::GetGraphicsContext();

		g_cubeBatchDatas.resize(MaxBatchCount);
		g_sphereBatchDatas.resize(MaxBatchCount);

		// cube
		std::vector<Vertex3D> vertices;
		std::vector<uint32_t> indices;
		//GenerateCubeMesh(vertices, indices);
		GenerateCube([&](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				vertices.push_back(vertex);
			},
			indices
		);

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
		GenerateSphere([&](vec3 pos, vec2 uv, vec3 normal, vec3 tangent, vec3 binormal) {
				Vertex3D vertex;
				vertex.position = pos;
				vertex.texcoord = uv;
				vertex.normal = normal;
				vertex.tangent = tangent;
				vertex.binormal = binormal;
				sphereVertices.push_back(vertex);
			}, 
			sphereIndices, 20, 20
		);

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

		// create constant buffers
		g_vpCB = context.CreateConstantBuffer(sizeof(VPMatrices));
		g_globalCB = context.CreateConstantBuffer(sizeof(GlobalConstants));
		g_lightCB = context.CreateConstantBuffer(sizeof(LightConstants));
		g_materialCB = context.CreateConstantBuffer(sizeof(MaterialConstants));	

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

		// Create a structured buffer for spot lights
		sbDesc.elmSize = sizeof(SpotLight);
		sbDesc.count = MaxSpotLights;
		sbDesc.accessFlags = AccessFlag::Write;
		sbDesc.bindFlags = BindFlag::ShaderResource;
		g_spotLightSB = context.CreateStructuredBuffer(sbDesc);

		// Create default pipeline
		g_std3dShader = context.CreateGraphicsShader("Resources/Shaders/std3d.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		g_std3dShader->AddInputElement<float>("POSITION", 3);
		g_std3dShader->AddInputElement<float>("TEXCOORD", 2);
		g_std3dShader->AddInputElement<float>("TANGENT", 3);
		g_std3dShader->AddInputElement<float>("NORMAL", 3);
		g_std3dShader->AddInputElement<float>("BINORMAL", 3);
		g_std3dShader->CreateInputLayout();

		g_pipeLine = context.CreateGraphicsPipeline();
	}

	void Renderer::Cleanup() {
		g_cubeIB.reset();
		g_cubeVB.reset();
		g_sphereIB.reset();
		g_sphereVB.reset();
		g_std3dShader.reset();
		g_vpCB.reset();
		g_lightCB.reset();
		g_materialCB.reset();
		g_batchDataSB.reset();
		g_directionalLightSB.reset();
		g_pointLightSB.reset();
		g_spotLightSB.reset();
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
		lightConstants.numSpotLights = std::min((uint32_t)env.spotLights.size(), MaxSpotLights);
		g_lightCB->Update(&lightConstants, sizeof(LightConstants));

		g_directionalLightSB->Update(env.directionalLights.data(), sizeof(DirectionalLight) * lightConstants.numDirectionalLights);	
		g_pointLightSB->Update(env.pointLights.data(), sizeof(PointLight) * lightConstants.numPointLights);
		g_spotLightSB->Update(env.spotLights.data(), sizeof(SpotLight) * lightConstants.numSpotLights);

		g_cubeBatchDatas.clear();
		g_sphereBatchDatas.clear();
	}

	void Renderer::End() {
		auto& cmdQueue = Graphics::GetCommandQueue();

		g_pipeLine->SetShader(g_std3dShader);
		g_pipeLine->SetBlendMode(BlendMode::Default);
		g_pipeLine->SetCullMode(CullMode::Back);
		g_pipeLine->SetDepthTest(DepthTest::Less);

		MaterialConstants mc;
		mc.reservedTextureBitMask = 0;
		g_materialCB->Update(&mc, sizeof(MaterialConstants));

		g_batchDataSB->Update(g_cubeBatchDatas.data(), sizeof(BatchedData) * g_cubeBatchDatas.size());

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_cubeVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);
		cmdQueue.SetConstantBuffer(g_materialCB, 3);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.SetStructuredBuffer(g_spotLightSB, 3);
		cmdQueue.DrawIndexedInstanced(g_cubeIB, g_cubeIB->IndexCount(), g_cubeBatchDatas.size());
		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();

		g_batchDataSB->Update(g_sphereBatchDatas.data(), sizeof(BatchedData) * g_sphereBatchDatas.size());

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_sphereVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);
		cmdQueue.SetConstantBuffer(g_materialCB, 3);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.SetStructuredBuffer(g_spotLightSB, 3);
		cmdQueue.DrawIndexedInstanced(g_sphereIB, g_sphereIB->IndexCount(), g_sphereBatchDatas.size());
		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();
	}

	void Renderer::DrawCube(const uint32_t id, const mat4& transform) {
		BatchedData data;
		data.transform = transform;
		data.id = id;
		g_cubeBatchDatas.push_back(std::move(data));
	}

	void Renderer::DrawCube(const uint32_t id, const mat4& transform, const Material& material) {
		// TODO: 배치 드로우를 생각해보자.
		auto& cmdQueue = Graphics::GetCommandQueue();

		g_pipeLine->SetCullMode(material.cullMode);
		g_pipeLine->SetDepthTest(material.depthTest, material.depthWrite);

		if (material.shader) {
			g_pipeLine->SetShader(material.shader);
		}
		else {
			g_pipeLine->SetShader(g_std3dShader);
		}

		BatchedData data;
		data.transform = transform;
		data.id = id;

		g_batchDataSB->Update(&data, sizeof(BatchedData));

		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_cubeVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);

		MaterialConstants mc;
		mc.reservedTextureBitMask = 0;
		if (material.albedoTexture) {
			mc.reservedTextureBitMask |= 0x1;
			cmdQueue.SetTexture(material.albedoTexture, ReservedTextureStartSlot);
		}

		if (material.normalTexture) {
			mc.reservedTextureBitMask |= 0x2;
			cmdQueue.SetTexture(material.normalTexture, ReservedTextureStartSlot + 1);
		}

		for (int32_t i = 0; i < material.cubeTextures.size(); ++i) {
			if (material.cubeTextures[i]) {
				mc.cubeTextureBitMask |= (1 << i);
				cmdQueue.SetTexture(material.cubeTextures[i], CubeTextureStartSlot + i);
			}
		}

		std::memcpy(mc.intConstants, material.intConstants, sizeof(uint32_t) * 4);
		g_materialCB->Update(&mc, sizeof(MaterialConstants));

		cmdQueue.SetConstantBuffer(g_materialCB, 3);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.SetStructuredBuffer(g_spotLightSB, 3);
		cmdQueue.DrawIndexedInstanced(g_cubeIB, g_cubeIB->IndexCount(), 1);
		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();
	}

	void Renderer::DrawSphere(const uint32_t id, const mat4& transform) {
		BatchedData data;
		data.transform = transform;
		data.id = id;
		g_sphereBatchDatas.push_back(std::move(data));
	}

	void Renderer::DrawSphere(const uint32_t id, const mat4& transform, const Material& material) {
		// TODO: 배치 드로우를 생각해보자.
		auto& cmdQueue = Graphics::GetCommandQueue();

		g_pipeLine->SetCullMode(material.cullMode);
		g_pipeLine->SetDepthTest(material.depthTest, material.depthWrite);

		if (material.shader) {
			g_pipeLine->SetShader(material.shader);
		}
		else {
			g_pipeLine->SetShader(g_std3dShader);
		}

		BatchedData data;
		data.transform = transform;
		data.id = id;

		g_batchDataSB->Update(&data, sizeof(BatchedData));
	
		cmdQueue.Begin();
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(g_pipeLine);
		cmdQueue.SetVertexBuffer(g_sphereVB);
		cmdQueue.SetConstantBuffer(g_vpCB, 0);
		cmdQueue.SetConstantBuffer(g_globalCB, 1);
		cmdQueue.SetConstantBuffer(g_lightCB, 2);

		MaterialConstants mc;

		mc.reservedTextureBitMask = 0;
		if (material.albedoTexture) {
			mc.reservedTextureBitMask |= 0x1;
			cmdQueue.SetTexture(material.albedoTexture, ReservedTextureStartSlot);
		}

		if (material.normalTexture) {
			mc.reservedTextureBitMask |= 0x2;
			cmdQueue.SetTexture(material.normalTexture, ReservedTextureStartSlot + 1);
		}
		
		for (int32_t i = 0; i < material.cubeTextures.size(); ++i) {
			if (material.cubeTextures[i]) {
				mc.cubeTextureBitMask |= (1 << i);
				cmdQueue.SetTexture(material.cubeTextures[i], CubeTextureStartSlot + i);
			}
		}

		std::memcpy(mc.intConstants, material.intConstants, sizeof(uint32_t) * 4);

		g_materialCB->Update(&mc, sizeof(MaterialConstants));
		
		cmdQueue.SetConstantBuffer(g_materialCB, 3);
		cmdQueue.SetStructuredBuffer(g_batchDataSB, 0);
		cmdQueue.SetStructuredBuffer(g_directionalLightSB, 1);
		cmdQueue.SetStructuredBuffer(g_pointLightSB, 2);
		cmdQueue.SetStructuredBuffer(g_spotLightSB, 3);
		cmdQueue.DrawIndexedInstanced(g_sphereIB, g_sphereIB->IndexCount(), 1);
		cmdQueue.ResetAllTextures();
		cmdQueue.End();

		cmdQueue.Execute();
	}
}
