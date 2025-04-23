#include "pch.h"
#include "SkyBoxSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "PrimitiveManager.h"

namespace flaw {
	SkyBoxSystem::SkyBoxSystem(Scene& scene)
		: _scene(scene) 
	{
		auto& context = Graphics::GetGraphicsContext();

		_skyBoxShader = context.CreateGraphicsShader("Resources/Shaders/skybox.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_skyBoxShader->AddInputElement<float>("POSITION", 3);
		_skyBoxShader->AddInputElement<float>("TEXCOORD", 2);
		_skyBoxShader->CreateInputLayout();

		_skyBoxMaterial = CreateRef<Material>();
		_skyBoxMaterial->renderMode = RenderMode::Masked;
		_skyBoxMaterial->shader = _skyBoxShader;
		_skyBoxMaterial->cullMode = CullMode::Front;
		_skyBoxMaterial->depthTest = DepthTest::LessEqual;
		_skyBoxMaterial->depthWrite = false;
	}

	void SkyBoxSystem::Update() {
		_skyBoxTexture2D.reset();
		_skyBoxTextureCube.reset();
		for (auto&& [entity, skyBox] : _scene.GetRegistry().view<SkyBoxComponent>().each()) {
			// SkyBoxComponent는 하나만 존재해야 함
			if (auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(skyBox.texture)) {
				_skyBoxTexture2D = tex2DAsset->GetTexture();
			}
			else if (auto texCubeAsset = AssetManager::GetAsset<TextureCubeAsset>(skyBox.texture)) {
				_skyBoxTextureCube = texCubeAsset->GetTexture();
			}

			break;
		}
	}

	void SkyBoxSystem::Render(
		Ref<GraphicsPipeline> pipeline, 
		Ref<ConstantBuffer> vpCB, 
		Ref<ConstantBuffer> globalCB, 
		Ref<ConstantBuffer> lightCB, 
		Ref<ConstantBuffer> materialCB,
		Ref<VertexBuffer> vertexBuffer,
		Ref<IndexBuffer> indexBuffer) 
	{
		if (!_skyBoxTexture2D && !_skyBoxTextureCube) {
			return;
		}

		Ref<Mesh> mesh;

		if (_skyBoxTexture2D) {
			_skyBoxMaterial->intConstants[0] = 0;
			_skyBoxMaterial->albedoTexture = _skyBoxTexture2D;
			mesh = PrimitiveManager::GetSphereMesh();
		}
		else if (_skyBoxTextureCube) {
			_skyBoxMaterial->intConstants[0] = 1;
			_skyBoxMaterial->cubeTextures[0] = _skyBoxTextureCube;
			mesh = PrimitiveManager::GetCubeMesh();
		}

		auto& cmdQueue = Graphics::GetCommandQueue();

		cmdQueue.Begin();

		// set pipeline
		pipeline->SetShader(_skyBoxMaterial->shader);
		pipeline->SetCullMode(_skyBoxMaterial->cullMode);
		pipeline->SetDepthTest(_skyBoxMaterial->depthTest, _skyBoxMaterial->depthWrite);

		cmdQueue.SetPipeline(pipeline);

		cmdQueue.SetConstantBuffer(vpCB, 0);
		cmdQueue.SetConstantBuffer(globalCB, 1);
		cmdQueue.SetConstantBuffer(lightCB, 2);

		// set material properties
		MaterialConstants materialConstants;

		materialConstants.reservedTextureBitMask = 0;
		if (_skyBoxMaterial->albedoTexture) {
			materialConstants.reservedTextureBitMask |= MaterialConstants::Albedo;
			cmdQueue.SetTexture(_skyBoxMaterial->albedoTexture, ReservedTextureStartSlot);
		}

		if (_skyBoxMaterial->normalTexture) {
			materialConstants.reservedTextureBitMask |= MaterialConstants::Normal;
			cmdQueue.SetTexture(_skyBoxMaterial->normalTexture, ReservedTextureStartSlot + 1);
		}

		if (_skyBoxMaterial->emissiveTexture) {
			materialConstants.reservedTextureBitMask |= MaterialConstants::Emissive;
			cmdQueue.SetTexture(_skyBoxMaterial->emissiveTexture, ReservedTextureStartSlot + 2);
		}

		if (_skyBoxMaterial->heightTexture) {
			materialConstants.reservedTextureBitMask |= MaterialConstants::Height;
			cmdQueue.SetTexture(_skyBoxMaterial->heightTexture, ReservedTextureStartSlot + 3);
		}

		materialConstants.cubeTextureBitMask = 0;
		for (int32_t i = 0; i < _skyBoxMaterial->cubeTextures.size(); ++i) {
			if (_skyBoxMaterial->cubeTextures[i]) {
				materialConstants.cubeTextureBitMask |= (1 << i);
				cmdQueue.SetTexture(_skyBoxMaterial->cubeTextures[i], CubeTextureStartSlot + i);
			}
		}

		std::memcpy(
			materialConstants.intConstants,
			_skyBoxMaterial->intConstants,
			sizeof(uint32_t) * 4 + sizeof(float) * 4
		);

		materialCB->Update(&materialConstants, sizeof(MaterialConstants));

		cmdQueue.SetConstantBuffer(materialCB, 3);

		cmdQueue.SetPrimitiveTopology(PrimitiveTopology::TriangleList);

		vertexBuffer->Update(mesh->vertices.data(), sizeof(Vertex3D), mesh->vertices.size());
		indexBuffer->Update(mesh->indices.data(), mesh->indices.size());

		cmdQueue.SetVertexBuffer(vertexBuffer);
		cmdQueue.DrawIndexed(indexBuffer, mesh->indices.size());

		cmdQueue.End();

		cmdQueue.Execute();
	}
}