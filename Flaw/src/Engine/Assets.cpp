#include "pch.h"
#include "Assets.h"
#include "Log/Log.h"
#include "Graphics/GraphicsFunc.h"
#include "Fonts.h"
#include "Sounds.h"
#include "AssetManager.h"

namespace flaw {
	void Texture2DAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		Texture2D::Descriptor desc = {};
		archive >> desc.format;
		archive >> desc.width;
		archive >> desc.height;
		archive >> desc.wrapS;
		archive >> desc.wrapT;
		archive >> desc.minFilter;
		archive >> desc.magFilter;
		archive >> desc.usage;
		archive >> desc.access;
		archive >> desc.bindFlags;

		std::vector<uint8_t> textureData;
		archive >> textureData;
		desc.data = textureData.data();

		_texture = Graphics::CreateTexture2D(desc);
	}

	void Texture2DAsset::Unload() {
		_texture.reset();
	}

	void Texture2DArrayAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		Texture2DArray::Descriptor desc = {};
		desc.fromMemory = true;
		archive >> desc.arraySize;
		archive >> desc.format;
		archive >> desc.width;
		archive >> desc.height;
		archive >> desc.usage;
		archive >> desc.access;
		archive >> desc.bindFlags;
		
		std::vector<uint8_t> textureData;
		archive >> textureData;
		desc.data = textureData.data();

		_texture = Graphics::CreateTexture2DArray(desc);
	}

	void Texture2DArrayAsset::Unload() {
		_texture.reset();
	}

	void TextureCubeAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		TextureCube::Descriptor desc = {};
		archive >> desc.format;
		archive >> desc.width;
		archive >> desc.height;
		archive >> desc.layout;

		std::vector<uint8_t> textureData;
		archive >> textureData;
		desc.data = textureData.data();

		_texture = Graphics::CreateTextureCube(desc);
	}

	void TextureCubeAsset::Unload() {
		_texture.reset();
	}

	void FontAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		std::vector<int8_t> fontData;
		archive >> fontData;

		_font = Fonts::CreateFontFromMemory(fontData.data(), fontData.size());

		FontAtlas atlas;
		archive >> atlas.width;
		archive >> atlas.height;
		archive >> atlas.data;

		Texture2D::Descriptor desc = {};
		desc.width = atlas.width;
		desc.height = atlas.height;
		desc.format = PixelFormat::RGBA8;
		desc.data = atlas.data.data();
		desc.usage = UsageFlag::Static;
		desc.bindFlags = BindFlag::ShaderResource;

		_fontAtlas = Graphics::CreateTexture2D(desc);
	}

	void FontAsset::Unload() {
		_fontAtlas.reset();
		_font.reset();
	}

	void SoundAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		std::vector<int8_t> soundData;
		archive >> soundData;

		_sound = Sounds::CreateSoundSourceFromMemory(soundData.data(), soundData.size());
	}

	void SoundAsset::Unload() {
		_sound.reset();
	}

	void MeshAsset::Load() {
		std::vector<int8_t> data;
		_getMemoryFunc(data);

		SerializationArchive archive(data.data(), data.size());

		auto& graphicsContext = Graphics::GetGraphicsContext();

		std::vector<int8_t> meshData;
		archive >> meshData;

		VertexBuffer::Descriptor vertexDesc = {};
		vertexDesc.usage = UsageFlag::Static;
		vertexDesc.elmSize = sizeof(Vertex3D);
		vertexDesc.bufferSize = meshData.size();
		vertexDesc.initialData = meshData.data();

		_vertexBuffer = graphicsContext.CreateVertexBuffer(vertexDesc);

		std::vector<uint32_t> indices;
		archive >> indices;

		IndexBuffer::Descriptor indexDesc = {};
		indexDesc.usage = UsageFlag::Static;
		indexDesc.bufferSize = indices.size() * sizeof(uint32_t);
		indexDesc.initialData = indices.data();

		_indexBuffer = graphicsContext.CreateIndexBuffer(indexDesc);
	}

	void MeshAsset::Unload() {
		_vertexBuffer.reset();
		_indexBuffer.reset();
	}

	void SkeletalMeshAsset::Load() {
		Descriptor desc;
		_getDesc(desc);

		_mesh = CreateRef<Mesh>(desc.vertices, desc.indices, desc.segments);
		_materials = std::move(desc.materials);
	}

	void SkeletalMeshAsset::Unload() {
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
	}

	void MaterialAsset::Unload() {
		_material.reset();
	}
}