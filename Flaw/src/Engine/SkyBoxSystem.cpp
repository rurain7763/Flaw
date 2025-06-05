#include "pch.h"
#include "SkyBoxSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"

namespace flaw {
	SkyBoxSystem::SkyBoxSystem(Scene& scene)
		: _scene(scene) 
	{
		_cubeMapShader = Graphics::CreateGraphicsShader("Resources/Shaders/cubemap.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		_cubeMapShader->AddInputElement<float>("POSITION", 3);
		_cubeMapShader->AddInputElement<float>("TEXCOORD", 2);
		_cubeMapShader->AddInputElement<float>("TANGENT", 3);
		_cubeMapShader->AddInputElement<float>("NORMAL", 3);
		_cubeMapShader->AddInputElement<float>("BINORMAL", 3);
		_cubeMapShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_cubeMapShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_cubeMapShader->CreateInputLayout();

		_irradianceShader = Graphics::CreateGraphicsShader("Resources/Shaders/skybox_irradiance.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		_irradianceShader->AddInputElement<float>("POSITION", 3);
		_irradianceShader->AddInputElement<float>("TEXCOORD", 2);
		_irradianceShader->AddInputElement<float>("TANGENT", 3);
		_irradianceShader->AddInputElement<float>("NORMAL", 3);
		_irradianceShader->AddInputElement<float>("BINORMAL", 3);
		_irradianceShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_irradianceShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_irradianceShader->CreateInputLayout();

		_prefilteredShader = Graphics::CreateGraphicsShader("Resources/Shaders/skybox_prefiltered.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Geometry | ShaderCompileFlag::Pixel);
		_prefilteredShader->AddInputElement<float>("POSITION", 3);
		_prefilteredShader->AddInputElement<float>("TEXCOORD", 2);
		_prefilteredShader->AddInputElement<float>("TANGENT", 3);
		_prefilteredShader->AddInputElement<float>("NORMAL", 3);
		_prefilteredShader->AddInputElement<float>("BINORMAL", 3);
		_prefilteredShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_prefilteredShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_prefilteredShader->CreateInputLayout();

		_brdfLUTShader = Graphics::CreateGraphicsShader("Resources/Shaders/skybox_brdflut.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_brdfLUTShader->AddInputElement<float>("POSITION", 3);
		_brdfLUTShader->AddInputElement<float>("TEXCOORD", 2);
		_brdfLUTShader->AddInputElement<float>("TANGENT", 3);
		_brdfLUTShader->AddInputElement<float>("NORMAL", 3);
		_brdfLUTShader->AddInputElement<float>("BINORMAL", 3);
		_brdfLUTShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_brdfLUTShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_brdfLUTShader->CreateInputLayout();

		_skyBoxShader = Graphics::CreateGraphicsShader("Resources/Shaders/skybox.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_skyBoxShader->AddInputElement<float>("POSITION", 3);
		_skyBoxShader->AddInputElement<float>("TEXCOORD", 2);
		_skyBoxShader->AddInputElement<float>("TANGENT", 3);
		_skyBoxShader->AddInputElement<float>("NORMAL", 3);
		_skyBoxShader->AddInputElement<float>("BINORMAL", 3);
		_skyBoxShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_skyBoxShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_skyBoxShader->CreateInputLayout();

		_skybox.brdfLUTTexture = GenerateBRDFLUTTexture();
	}

	void SkyBoxSystem::Update() {
		for (auto&& [entity, skyBoxComp] : _scene.GetRegistry().view<SkyBoxComponent>().each()) {
			// SkyBoxComponent는 하나만 존재해야 함
			Ref<Texture> prevTexture = _skybox.originalTexture;

			if (auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(skyBoxComp.texture)) {
				_skybox.originalTexture = tex2DAsset->GetTexture();

				if (prevTexture != _skybox.originalTexture) {
					_skybox.cubemapTexture = GenerateCubemap(tex2DAsset->GetTexture());
				}
			}
			else if (auto texCubeAsset = AssetManager::GetAsset<TextureCubeAsset>(skyBoxComp.texture)) {
				_skybox.originalTexture = texCubeAsset->GetTexture();
				_skybox.cubemapTexture = texCubeAsset->GetTexture();
				_skybox.cubemapTexture->GenerateMips(CubeMipLevels);
			}
			
			if (prevTexture != _skybox.originalTexture) {
				_skybox.irradianceTexture = GenerateIrradianceCubemap(_skybox.cubemapTexture);
				_skybox.prefilteredTexture = GeneratePrefilteredCubemap(_skybox.cubemapTexture);
			}

			break;
		}
	}

	Ref<TextureCube> SkyBoxSystem::GenerateCubemap(Ref<Texture2D> texture) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		TextureCube::Descriptor desc = {};
		desc.width = CubeTextureSize;
		desc.height = CubeTextureSize;
		desc.usage = UsageFlag::Static;
		desc.layout = TextureCube::Layout::Horizontal;
		desc.format = PixelFormat::RGBA32F;
		desc.bindFlags = BindFlag::ShaderResource | BindFlag::RenderTarget;

		auto cubeTexture = Graphics::CreateTextureCube(desc);

		GraphicsRenderPass::Descriptor cubeMapPassDesc = {};
		cubeMapPassDesc.renderTargets.resize(1);

		cubeMapPassDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		cubeMapPassDesc.renderTargets[0].alphaToCoverage = false;
		cubeMapPassDesc.renderTargets[0].viewportX = 0;
		cubeMapPassDesc.renderTargets[0].viewportY = 0;
		cubeMapPassDesc.renderTargets[0].viewportWidth = CubeTextureSize;
		cubeMapPassDesc.renderTargets[0].viewportHeight = CubeTextureSize;
		cubeMapPassDesc.renderTargets[0].texture = cubeTexture;
		cubeMapPassDesc.renderTargets[0].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto cubeMapRenderPass = Graphics::CreateRenderPass(cubeMapPassDesc);

		struct VPMatrix {
			mat4 view;
			mat4 projection;
		};

		mat4 perspective = Perspective(glm::half_pi<float>(), 1.0f, 0.1f, 10.0f);

		std::vector<VPMatrix> vpMatrices = {
			{ LookAt(vec3(0.0), Right, Up), perspective }, // Right
			{ LookAt(vec3(0.0), -Right, Up), perspective }, // Left
			{ LookAt(vec3(0.0), Up, -Forward), perspective }, // Up
			{ LookAt(vec3(0.0), -Up, Forward), perspective }, // Down
			{ LookAt(vec3(0.0), Forward, Up), perspective }, // Forward
			{ LookAt(vec3(0.0), -Forward, Up), perspective }  // Backward
		};

		StructuredBuffer::Descriptor vpMatricesDesc = {};
		vpMatricesDesc.elmSize = sizeof(VPMatrix);
		vpMatricesDesc.count = 6;
		vpMatricesDesc.bindFlags = BindFlag::ShaderResource;
		vpMatricesDesc.initialData = vpMatrices.data();
		
		auto vpMatricesSB = Graphics::CreateStructuredBuffer(vpMatricesDesc);
		auto mesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cube_mesh")->GetMesh();

		cubeMapRenderPass->Bind();

		pipeline->SetShader(_cubeMapShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetStructuredBuffer(vpMatricesSB, 0);
		cmdQueue.SetTexture(texture, 1);
		cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexed(mesh->GetGPUIndexBuffer(), mesh->GetGPUIndexBuffer()->IndexCount());
		cmdQueue.Execute();

		cubeMapRenderPass->Unbind();

		cubeTexture->GenerateMips(CubeMipLevels);

		return cubeTexture;
	}

	Ref<TextureCube> SkyBoxSystem::GenerateIrradianceCubemap(Ref<TextureCube> cubemap) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		TextureCube::Descriptor desc = {};
		desc.width = IrradianceTextureSize;
		desc.height = IrradianceTextureSize;
		desc.usage = UsageFlag::Static;
		desc.layout = TextureCube::Layout::Horizontal;
		desc.format = PixelFormat::RGBA32F;
		desc.bindFlags = BindFlag::ShaderResource | BindFlag::RenderTarget;

		auto cubeTexture = Graphics::CreateTextureCube(desc);

		GraphicsRenderPass::Descriptor irradiancePassDesc = {};
		irradiancePassDesc.renderTargets.resize(1);

		irradiancePassDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		irradiancePassDesc.renderTargets[0].alphaToCoverage = false;
		irradiancePassDesc.renderTargets[0].viewportX = 0;
		irradiancePassDesc.renderTargets[0].viewportY = 0;
		irradiancePassDesc.renderTargets[0].viewportWidth = IrradianceTextureSize;
		irradiancePassDesc.renderTargets[0].viewportHeight = IrradianceTextureSize;
		irradiancePassDesc.renderTargets[0].texture = cubeTexture;
		irradiancePassDesc.renderTargets[0].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto irraianceRenderPass = Graphics::CreateRenderPass(irradiancePassDesc);

		struct VPMatrix {
			mat4 view;
			mat4 projection;
		};

		mat4 perspective = Perspective(glm::half_pi<float>(), 1.0f, 0.1f, 10.0f);

		std::vector<VPMatrix> vpMatrices = {
			{ LookAt(vec3(0.0), Right, Up), perspective }, // Right
			{ LookAt(vec3(0.0), -Right, Up), perspective }, // Left
			{ LookAt(vec3(0.0), Up, -Forward), perspective }, // Up
			{ LookAt(vec3(0.0), -Up, Forward), perspective }, // Down
			{ LookAt(vec3(0.0), Forward, Up), perspective }, // Forward
			{ LookAt(vec3(0.0), -Forward, Up), perspective }  // Backward
		};

		StructuredBuffer::Descriptor vpMatricesDesc = {};
		vpMatricesDesc.elmSize = sizeof(VPMatrix);
		vpMatricesDesc.count = 6;
		vpMatricesDesc.bindFlags = BindFlag::ShaderResource;
		vpMatricesDesc.initialData = vpMatrices.data();

		auto vpMatricesSB = Graphics::CreateStructuredBuffer(vpMatricesDesc);
		auto mesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cube_mesh")->GetMesh();

		irraianceRenderPass->Bind();

		pipeline->SetShader(_irradianceShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::Disabled);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetStructuredBuffer(vpMatricesSB, 0);
		cmdQueue.SetTexture(cubemap, 1);
		cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexed(mesh->GetGPUIndexBuffer(), mesh->GetGPUIndexBuffer()->IndexCount());
		cmdQueue.Execute();

		irraianceRenderPass->Unbind();

		return cubeTexture;
	}

	Ref<TextureCube> SkyBoxSystem::GeneratePrefilteredCubemap(Ref<TextureCube> cubemap) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		TextureCube::Descriptor desc = {};
		desc.width = PrefilteredTextureSize;
		desc.height = PrefilteredTextureSize;
		desc.usage = UsageFlag::Static;
		desc.layout = TextureCube::Layout::Horizontal;
		desc.format = PixelFormat::RGBA32F;
		desc.bindFlags = BindFlag::ShaderResource | BindFlag::RenderTarget;

		auto cubeTexture = Graphics::CreateTextureCube(desc);
		cubeTexture->GenerateMips(PrefilteredMipLevels);

		GraphicsRenderPass::Descriptor prefilteredPassDesc = {};
		prefilteredPassDesc.renderTargets.resize(1);

		prefilteredPassDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		prefilteredPassDesc.renderTargets[0].alphaToCoverage = false;
		prefilteredPassDesc.renderTargets[0].viewportX = 0;
		prefilteredPassDesc.renderTargets[0].viewportY = 0;
		prefilteredPassDesc.renderTargets[0].viewportWidth = IrradianceTextureSize;
		prefilteredPassDesc.renderTargets[0].viewportHeight = IrradianceTextureSize;
		prefilteredPassDesc.renderTargets[0].texture = cubeTexture;
		prefilteredPassDesc.renderTargets[0].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto prefilteredRP = Graphics::CreateRenderPass(prefilteredPassDesc);

		struct PrefilteredUniforms {
			float roughness;
			float skyBoxTextureSize;
			float padding[2];
		};

		auto prefilteredUniformsCB = Graphics::CreateConstantBuffer(sizeof(PrefilteredUniforms));

		struct VPMatrix {
			mat4 view;
			mat4 projection;
		};

		mat4 perspective = Perspective(glm::half_pi<float>(), 1.0f, 0.1f, 10.0f);

		std::vector<VPMatrix> vpMatrices = {
			{ LookAt(vec3(0.0), Right, Up), perspective }, // Right
			{ LookAt(vec3(0.0), -Right, Up), perspective }, // Left
			{ LookAt(vec3(0.0), Up, -Forward), perspective }, // Up
			{ LookAt(vec3(0.0), -Up, Forward), perspective }, // Down
			{ LookAt(vec3(0.0), Forward, Up), perspective }, // Forward
			{ LookAt(vec3(0.0), -Forward, Up), perspective }  // Backward
		};

		StructuredBuffer::Descriptor vpMatricesDesc = {};
		vpMatricesDesc.elmSize = sizeof(VPMatrix);
		vpMatricesDesc.count = 6;
		vpMatricesDesc.bindFlags = BindFlag::ShaderResource;
		vpMatricesDesc.initialData = vpMatrices.data();

		auto vpMatricesSB = Graphics::CreateStructuredBuffer(vpMatricesDesc);
		auto mesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cube_mesh")->GetMesh();

		for (int32_t i = 0; i < PrefilteredMipLevels; i++) {
			const uint32_t mipSize = PrefilteredTextureSize >> i;

			prefilteredRP->SetViewport(0, 0, 0, mipSize, mipSize);
			prefilteredRP->SetRenderTargetMipLevel(0, i);

			prefilteredRP->Bind();

			PrefilteredUniforms prefilteredUniforms;
			prefilteredUniforms.roughness = float(i) / float(PrefilteredMipLevels - 1);
			prefilteredUniforms.skyBoxTextureSize = float(cubeTexture->GetWidth());
			prefilteredUniformsCB->Update(&prefilteredUniforms, sizeof(PrefilteredUniforms));

			pipeline->SetShader(_prefilteredShader);
			pipeline->SetFillMode(FillMode::Solid);
			pipeline->SetCullMode(CullMode::Front);
			pipeline->SetDepthTest(DepthTest::Disabled);

			cmdQueue.SetPipeline(pipeline);
			cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
			cmdQueue.SetConstantBuffer(prefilteredUniformsCB, 0);
			cmdQueue.SetStructuredBuffer(vpMatricesSB, 0);
			cmdQueue.SetTexture(cubemap, 1);
			cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
			cmdQueue.DrawIndexed(mesh->GetGPUIndexBuffer(), mesh->GetGPUIndexBuffer()->IndexCount());
			cmdQueue.Execute();

			prefilteredRP->Unbind();
		}

		return cubeTexture;
	}

	Ref<Texture2D> SkyBoxSystem::GenerateBRDFLUTTexture() {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		Texture2D::Descriptor desc = {};
		desc.width = BRDFLUTTextureSize;
		desc.height = BRDFLUTTextureSize;
		desc.usage = UsageFlag::Static;
		desc.format = PixelFormat::RGBA8;
		desc.bindFlags = BindFlag::ShaderResource | BindFlag::RenderTarget;

		auto brdfLUTTexture = Graphics::CreateTexture2D(desc);

		GraphicsRenderPass::Descriptor brdfLUTPassDesc = {};
		brdfLUTPassDesc.renderTargets.resize(1);
		brdfLUTPassDesc.renderTargets[0].blendMode = BlendMode::Disabled;
		brdfLUTPassDesc.renderTargets[0].alphaToCoverage = false;
		brdfLUTPassDesc.renderTargets[0].viewportX = 0;
		brdfLUTPassDesc.renderTargets[0].viewportY = 0;
		brdfLUTPassDesc.renderTargets[0].viewportWidth = BRDFLUTTextureSize;
		brdfLUTPassDesc.renderTargets[0].viewportHeight = BRDFLUTTextureSize;
		brdfLUTPassDesc.renderTargets[0].texture = brdfLUTTexture;
		brdfLUTPassDesc.renderTargets[0].clearValue = { 0.0f, 0.0f, 0.0f, 0.0f };

		auto brdfLUTRenderPass = Graphics::CreateRenderPass(brdfLUTPassDesc);

		auto mesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_quad_mesh")->GetMesh();

		brdfLUTRenderPass->Bind();

		pipeline->SetShader(_brdfLUTShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Back);
		pipeline->SetDepthTest(DepthTest::Disabled);

		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexed(mesh->GetGPUIndexBuffer(), mesh->GetGPUIndexBuffer()->IndexCount());
		cmdQueue.Execute();

		return brdfLUTTexture;
	}

	void SkyBoxSystem::Render(Ref<ConstantBuffer> vpCB) {
		if (!_skybox.cubemapTexture) {
			return;
		}

		auto& mainPass = Graphics::GetMainRenderPass();
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		auto mesh = AssetManager::GetAsset<StaticMeshAsset>("default_static_cube_mesh")->GetMesh();

		mainPass->SetBlendMode(0, BlendMode::Disabled, false);
		mainPass->Bind(false, false);

		// set pipeline
		pipeline->SetShader(_skyBoxShader);
		pipeline->SetFillMode(FillMode::Solid);
		pipeline->SetCullMode(CullMode::Front);
		pipeline->SetDepthTest(DepthTest::LessEqual, false);

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);
		cmdQueue.SetPipeline(pipeline);
		cmdQueue.SetConstantBuffer(vpCB, 0);
		cmdQueue.SetTexture(_skybox.cubemapTexture, 0);
		cmdQueue.SetVertexBuffer(mesh->GetGPUVertexBuffer());
		cmdQueue.DrawIndexed(mesh->GetGPUIndexBuffer(), mesh->GetGPUIndexBuffer()->IndexCount());

		cmdQueue.Execute();
	}
}