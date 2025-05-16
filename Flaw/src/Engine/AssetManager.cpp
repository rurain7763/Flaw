#include "pch.h"
#include "AssetManager.h"
#include "Application.h"
#include "Log/Log.h"
#include "Image/Image.h"
#include "Graphics/Texture.h"
#include "Graphics.h"
#include "Serialization.h"

namespace flaw {
	static std::unordered_map<std::string, AssetHandle> g_assetKeyMap;
	static std::unordered_map<AssetHandle, Ref<Asset>> g_registeredAssets;

	void AssetManager::Init() {
		RegisterDefaultGraphicsShader();
	}

	void AssetManager::RegisterDefaultGraphicsShader() {
		AssetHandle handle = g_registeredAssets.size();
		auto graphicsShaderAsset = CreateRef<GraphicsShaderAsset>(
			[](GraphicsShaderAsset::Descriptor& desc) {
				desc.shaderPath = "Resources/Shaders/std3d_geometry.fx";
				desc.shaderCompileFlags = ShaderCompileFlag::Vertex | ShaderCompileFlag::Pixel;
				desc.inputElements.push_back({ "POSITION", GraphicsShader::InputElement::ElementType::Float, 3, false });
				desc.inputElements.push_back({ "TEXCOORD", GraphicsShader::InputElement::ElementType::Float, 2, false });
				desc.inputElements.push_back({ "TANGENT", GraphicsShader::InputElement::ElementType::Float, 3, false });
				desc.inputElements.push_back({ "NORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
				desc.inputElements.push_back({ "BINORMAL", GraphicsShader::InputElement::ElementType::Float, 3, false });
			}
		);

		RegisterAsset(handle, graphicsShaderAsset);
		RegisterKey("std3d_geometry", handle);
	}

	void AssetManager::Cleanup() {
		g_registeredAssets.clear();
	}

	void AssetManager::RegisterKey(const std::string_view& key, const AssetHandle& handle) {
		if (g_assetKeyMap.find(key.data()) != g_assetKeyMap.end()) {
			return;
		}
		g_assetKeyMap[key.data()] = handle;
	}

	void AssetManager::UnregisterKey(const std::string_view& key, const AssetHandle& handle) {
		auto it = g_assetKeyMap.find(key.data());
		if (it == g_assetKeyMap.end()) {
			return;
		}
		g_assetKeyMap.erase(it);
	}

	AssetHandle AssetManager::GetHandleByKey(const std::string_view& key) {
		auto it = g_assetKeyMap.find(key.data());
		if (it == g_assetKeyMap.end()) {
			return AssetHandle();
		}
		return it->second;
	}

	void AssetManager::RegisterAsset(const AssetHandle& handle, const Ref<Asset>& asset) {
		if (IsAssetRegistered(handle)) {
			return;
		}

		// NOTE: Only register the asset, do not load it yet
		g_registeredAssets[handle] = asset;
	}

	void AssetManager::UnregisterAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		g_registeredAssets.erase(it);
	}

	void AssetManager::LoadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (it->second->IsLoaded()) {
			return;
		}

		it->second->Load();
	}

	void AssetManager::UnloadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (!it->second->IsLoaded()) {
			return;
		}

		it->second->Unload();
	}

	void AssetManager::ReloadAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return;
		}

		if (it->second->IsLoaded()) {
			it->second->Unload();
		}

		it->second->Load();
	}

	Ref<Asset> AssetManager::GetAsset(const AssetHandle& handle) {
		auto it = g_registeredAssets.find(handle);
		if (it == g_registeredAssets.end()) {
			return nullptr;
		}

		if (!it->second->IsLoaded()) {
			it->second->Load();
		}

		return it->second;
	}

	bool AssetManager::IsAssetRegistered(const AssetHandle& handle) {
		return g_registeredAssets.find(handle) != g_registeredAssets.end();
	}

	AssetHandle AssetManager::GenerateNewAssetHandle() {
		AssetHandle handle;
		handle.Generate();

		while (IsAssetRegistered(handle)) {
			handle.Generate();
		}

		return handle;
	}
}
