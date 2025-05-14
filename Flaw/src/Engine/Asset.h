#pragma once

#include "Core.h"
#include "Utils/UUID.h"
#include "Utils/SerializationArchive.h"

namespace flaw {
	using AssetHandle = UUID;

	template <>
	struct Serializer<AssetHandle> {
		static void Serialize(SerializationArchive& archive, const AssetHandle& value) {
			archive << (uint64_t)value;
		}

		static void Deserialize(SerializationArchive& archive, AssetHandle& value) {
			uint64_t handle;
			archive >> handle;
			value = handle;
		}
	};

	enum class AssetType : int32_t {
		Unknown = 0,
		Reserved,
		Texture2D,
		Font,
		Sound,
		Mesh,
		TextureCube,
		Texture2DArray,
		SkeletalMesh,
	};

	class Asset {
	public:
		virtual ~Asset() = default;

		virtual void Load() = 0;
		virtual void Unload() = 0;

		virtual AssetType GetAssetType() const = 0;
		virtual bool IsLoaded() const = 0;

	private:
		AssetHandle _handle;
	};
}