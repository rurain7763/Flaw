#include "pch.h"
#include "SkyBoxSystem.h"
#include "Scene.h"
#include "Components.h"
#include "AssetManager.h"
#include "Assets.h"
#include "Renderer.h"

namespace flaw {
	SkyBoxSystem::SkyBoxSystem(Scene& scene)
		: _scene(scene) 
	{
		auto& context = Graphics::GetGraphicsContext();

		_skyBoxShader = context.CreateGraphicsShader("Resources/Shaders/skybox.fx", ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel);
		_skyBoxShader->AddInputElement<float>("POSITION", 3);
		_skyBoxShader->AddInputElement<float>("TEXCOORD", 2);
		_skyBoxShader->CreateInputLayout();
	}

	void SkyBoxSystem::Update() {
		auto& enttRegistry = _scene.GetRegistry();

		_skyBoxTexture2D.reset();
		_skyBoxTextureCube.reset();
		for (auto&& [entity, skyBox] : enttRegistry.view<SkyBoxComponent>().each()) {
			if (auto tex2DAsset = AssetManager::GetAsset<Texture2DAsset>(skyBox.texture)) {
				_skyBoxTexture2D = tex2DAsset->GetTexture();
			}
			else if (auto texCubeAsset = AssetManager::GetAsset<TextureCubeAsset>(skyBox.texture)) {
				_skyBoxTextureCube = texCubeAsset->GetTexture();
			}

			// SkyBoxComponent는 하나만 존재해야 함
			break;
		}
	}

	void SkyBoxSystem::Render() {
		if (!_skyBoxTexture2D && !_skyBoxTextureCube) {
			return;
		}

		Material material;
		material.cullMode = CullMode::Front;
		material.depthTest = DepthTest::LessEqual;
		material.depthWrite = false;
		material.shader = _skyBoxShader;

		if (_skyBoxTexture2D) {
			material.intConstants[0] = 0;
			material.albedoTexture = _skyBoxTexture2D;

			// TODO: view, proj은 어떻게 처리할지 생각해보기 카메라의 회전량만 알면 될듯
			Renderer::DrawSphere((uint32_t)entt::null, mat4(1.0f), material);
		}
		else if (_skyBoxTextureCube) {
			material.intConstants[0] = 1;
			material.cubeTextures[0] = _skyBoxTextureCube;

			Renderer::DrawCube((uint32_t)entt::null, mat4(1.0f), material);
		}
	}
}