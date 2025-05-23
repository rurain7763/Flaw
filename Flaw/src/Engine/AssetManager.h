#pragma once

#include "Core.h"
#include "Asset.h"

namespace flaw {
	class Application;

	class AssetManager {
	public:
		static void Init();
		static void Cleanup();

		static void RegisterKey(const std::string_view& key, const AssetHandle& handle);
		static void UnregisterKey(const std::string_view& key, const AssetHandle& handle);
		static AssetHandle GetHandleByKey(const std::string_view& key);

		static void RegisterAsset(const AssetHandle& handle, const Ref<Asset>& asset);
		static void UnregisterAsset(const AssetHandle& handle);

		static void LoadAsset(const AssetHandle& handle);
		static void UnloadAsset(const AssetHandle& handle);
		static void ReloadAsset(const AssetHandle& handle);

		static Ref<Asset> GetAsset(const AssetHandle& handle);
		static Ref<Asset> GetAsset(const std::string_view& key);

		static bool IsAssetRegistered(const AssetHandle& handle);

		template <typename T>
		static Ref<T> GetAsset(const AssetHandle& handle) {
			return std::dynamic_pointer_cast<T>(GetAsset(handle));
		}

		template <typename T>
		static Ref<T> GetAsset(const std::string_view& key) {
			return std::dynamic_pointer_cast<T>(GetAsset(key));
		}
		
		static AssetHandle GenerateNewAssetHandle();

	private: 
		static void RegisterDefaultGraphicsShaders();
		static void RegisterDefaultMaterials();
		static void RegisterDefaultStaticMeshs();
	};
}

