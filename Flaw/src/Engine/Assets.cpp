#include "pch.h"
#include "Assets.h"
#include "Log/Log.h"
#include "Graphics/GraphicsFunc.h"
#include "Fonts.h"
#include "Sounds.h"
#include "AssetManager.h"

namespace flaw {
	void Texture2DAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		Texture2D::Descriptor texDesc = {};
		texDesc.format = desc.format;
		texDesc.width = desc.width;
		texDesc.height = desc.height;
		texDesc.wrapS = Texture2D::Wrap::ClampToEdge;
		texDesc.wrapT = Texture2D::Wrap::ClampToEdge;
		texDesc.minFilter = Texture2D::Filter::Linear;
		texDesc.magFilter = Texture2D::Filter::Linear;
		texDesc.usage = desc.usage;
		texDesc.access = desc.accessFlags;
		texDesc.bindFlags = desc.bindFlags;
		texDesc.data = desc.data.data();

		_texture = Graphics::CreateTexture2D(texDesc);
	}

	void Texture2DAsset::Unload() {
		_texture.reset();
	}

	void Texture2DArrayAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		Texture2DArray::Descriptor texDesc = {};
		texDesc.fromMemory = true;
		texDesc.arraySize = desc.arraySize;
		texDesc.format = desc.format;
		texDesc.width = desc.width;
		texDesc.height = desc.height;
		texDesc.usage = desc.usage;
		texDesc.access = desc.accessFlags;
		texDesc.bindFlags = desc.bindFlags;
		texDesc.data = desc.data.data();

		_texture = Graphics::CreateTexture2DArray(texDesc);
	}

	void Texture2DArrayAsset::Unload() {
		_texture.reset();
	}

	void TextureCubeAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		TextureCube::Descriptor texDesc = {};
		texDesc.format = desc.format;
		texDesc.width = desc.width;
		texDesc.height = desc.height;
		texDesc.layout = desc.layout;
		texDesc.data = desc.data.data();
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::ShaderResource;
		texDesc.access = 0;

		_texture = Graphics::CreateTextureCube(texDesc);
	}

	void TextureCubeAsset::Unload() {
		_texture.reset();
	}

	void FontAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_font = Fonts::CreateFontFromMemory(desc.fontData.data(), desc.fontData.size());

		Texture2D::Descriptor texDesc = {};
		texDesc.width = desc.width;
		texDesc.height = desc.height;
		texDesc.format = PixelFormat::RGBA8;
		texDesc.data = desc.atlasData.data();
		texDesc.usage = UsageFlag::Static;
		texDesc.bindFlags = BindFlag::ShaderResource;

		_fontAtlas = Graphics::CreateTexture2D(texDesc);
	}

	void FontAsset::Unload() {
		_fontAtlas.reset();
		_font.reset();
	}

	void SoundAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_sound = Sounds::CreateSoundSourceFromMemory(desc.soundData.data(), desc.soundData.size());
	}

	void SoundAsset::Unload() {
		_sound.reset();
	}

	void StaticMeshAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_mesh = CreateRef<Mesh>(desc.vertices, desc.indices, desc.segments);
		_materials = std::move(desc.materials);
	}

	void StaticMeshAsset::Unload() {
		_materials.clear();
		_mesh.reset();
	}

	void SkeletalMeshAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_mesh = CreateRef<Mesh>(desc.vertices, desc.indices, desc.segments);
		_materials = std::move(desc.materials);
		_skeleton = desc.skeleton;
	}

	void SkeletalMeshAsset::Unload() {
		_skeleton.Invalidate();
		_materials.clear();
		_mesh.reset();
	}

	void GraphicsShaderAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_shader = Graphics::CreateGraphicsShader(desc.shaderPath.c_str(), desc.shaderCompileFlags);
		for (auto& inputElement : desc.inputElements) {
			_shader->AddInputElement(inputElement);
		}
		_shader->CreateInputLayout();
	}

	void GraphicsShaderAsset::Unload() {
		_shader.reset();
	}

	void MaterialAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_material = CreateRef<Material>();

		_material->renderMode = desc.renderMode;
		_material->cullMode = desc.cullMode;
		_material->depthTest = desc.depthTest;
		_material->depthWrite = desc.depthWrite;

		auto graphicsShaderAsset = AssetManager::GetAsset<GraphicsShaderAsset>(desc.shaderHandle);
		if (graphicsShaderAsset) {
			_material->shader = graphicsShaderAsset->GetShader();
		}

		auto texture2DAsset = AssetManager::GetAsset<Texture2DAsset>(desc.albedoTexture);
		if (texture2DAsset) {
			_material->albedoTexture = texture2DAsset->GetTexture();
		}

		auto normalTextureAsset = AssetManager::GetAsset<Texture2DAsset>(desc.normalTexture);
		if (normalTextureAsset) {
			_material->normalTexture = normalTextureAsset->GetTexture();
		}

		auto emissiveTextureAsset = AssetManager::GetAsset<Texture2DAsset>(desc.emissiveTexture);
		if (emissiveTextureAsset) {
			_material->emissiveTexture = emissiveTextureAsset->GetTexture();
		}

		auto metallicTextureAsset = AssetManager::GetAsset<Texture2DAsset>(desc.metallicTexture);
		if (metallicTextureAsset) {
			_material->metallicTexture = metallicTextureAsset->GetTexture();
		}

		auto roughnessTextureAsset = AssetManager::GetAsset<Texture2DAsset>(desc.roughnessTexture);
		if (roughnessTextureAsset) {
			_material->roughnessTexture = roughnessTextureAsset->GetTexture();
		}

		auto ambientOcclusionTextureAsset = AssetManager::GetAsset<Texture2DAsset>(desc.ambientOcclusionTexture);
		if (ambientOcclusionTextureAsset) {
			_material->ambientOcclusionTexture = ambientOcclusionTextureAsset->GetTexture();
		}
	}

	void MaterialAsset::Unload() {
		_material.reset();
	}

	void SkeletonAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_skeleton = CreateRef<Skeleton>(desc.globalInvMatrix, desc.nodes, desc.boneMap);
		_animationHandles = desc.animationHandles;
	}

	void SkeletonAsset::Unload() {
		_animationHandles.clear();
		_skeleton.reset();
	}

	void SkeletalAnimationAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_animation = CreateRef<SkeletalAnimation>(desc.name, desc.durationSec, desc.animationNodes);
	}

	void SkeletalAnimationAsset::Unload() {
		_animation.reset();
	}

	void PrefabAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_prefab = CreateRef<Prefab>(desc.prefabData.data());
	}

	void PrefabAsset::Unload() {
		_prefab.reset();
	}
}