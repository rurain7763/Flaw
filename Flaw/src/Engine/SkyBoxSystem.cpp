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

		_skyBoxShader = Graphics::CreateGraphicsShader("Resources/Shaders/skybox.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_skyBoxShader->AddInputElement<float>("POSITION", 3);
		_skyBoxShader->AddInputElement<float>("TEXCOORD", 2);
		_skyBoxShader->AddInputElement<float>("TANGENT", 3);
		_skyBoxShader->AddInputElement<float>("NORMAL", 3);
		_skyBoxShader->AddInputElement<float>("BINORMAL", 3);
		_skyBoxShader->AddInputElement<int32_t>("BONEINDICES", 4);
		_skyBoxShader->AddInputElement<float>("BONEWEIGHTS", 4);
		_skyBoxShader->CreateInputLayout();
	}

	void SkyBoxSystem::Update() {
		for (auto&& [entity, skyBoxComp] : _scene.GetRegistry().view<SkyBoxComponent>().each()) {
			// SkyBoxComponent는 하나만 존재해야 함
			Ref<Texture> prevTexture = _skybox.originalTexture;

			if (auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(skyBoxComp.texture)) {
				_skybox.originalTexture = tex2DAsset->GetTexture();

				if (prevTexture != _skybox.originalTexture) {
					_skybox.cubemapTexture = MakeCubemapFromTexture2D(tex2DAsset->GetTexture());
				}
			}
			else if (auto texCubeAsset = AssetManager::GetAsset<TextureCubeAsset>(skyBoxComp.texture)) {
				_skybox.originalTexture = texCubeAsset->GetTexture();
				_skybox.cubemapTexture = texCubeAsset->GetTexture();
			}
			
			if (prevTexture != _skybox.originalTexture) {
				_skybox.irradianceTexture = MakeIrradianceCubemap(_skybox.cubemapTexture);
			}

			break;
		}
	}

	Ref<TextureCube> SkyBoxSystem::MakeCubemapFromTexture2D(Ref<Texture2D> texture) {
		auto& cmdQueue = Graphics::GetCommandQueue();
		auto& pipeline = Graphics::GetMainGraphicsPipeline();

		TextureCube::Descriptor desc = {};
		desc.width = CubeTextureSize;
		desc.height = CubeTextureSize;
		desc.usage = UsageFlag::Static;
		desc.layout = TextureCube::Layout::Horizontal;
		desc.format = PixelFormat::RGBA8;
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

		return cubeTexture;
	}

	Ref<TextureCube> SkyBoxSystem::MakeIrradianceCubemap(Ref<TextureCube> cubemap) {
		// TODO: Implement irradiance cubemap generation
		return nullptr;
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