#include "pch.h"
#include "AssetManager.h"
#include "Application.h"
#include "Log/Log.h"
#include "Image/Image.h"
#include "Graphics/Texture.h"
#include "Graphics.h"
#include "Serialization.h"

namespace flaw {
	static std::unordered_map<AssetHandle, Ref<Asset>> g_registeredAssets;

	void AssetManager::Init() {
		// TODO: register initial assets
	}

	void AssetManager::Cleanup() {
		g_registeredAssets.clear();
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
}
