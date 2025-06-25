using System;

namespace Flaw
{
    public readonly struct AssetHandle
    {
        public readonly ulong handle;

        public AssetHandle(ulong handle)
        {
            this.handle = handle;
        }

        public static AssetHandle Invalid => new AssetHandle(ulong.MaxValue);

        public static implicit operator AssetHandle(ulong handle) => new AssetHandle(handle);
        public static implicit operator ulong(AssetHandle assetHandle) => assetHandle.handle;
    }

    public class Asset
    {
        internal AssetHandle handle;

        internal Asset()
        {
            handle = AssetHandle.Invalid;
        }

        internal Asset(AssetHandle handle)
        {
            this.handle = handle;
        }

        public ulong Handle
        {
            get => handle;
        }
    }

    public class Prefab : Asset
    {
        public Entity Instantiate()
        {
            if (handle == ulong.MaxValue)
            {
                throw new InvalidOperationException("Prefab handle is not set.");
            }

            ulong entityHandle = InternalCalls.CreateEntity_Prefab(handle);
            
            return new Entity(entityHandle);
        }

        public Entity Instantiate(Vec3 position)
        {
            if (handle == ulong.MaxValue)
            {
                throw new InvalidOperationException("Prefab handle is not set.");
            }

            Vec3 rotation = Vec3.Zero;
            Vec3 scale = Vec3.One;
            ulong entityHandle = InternalCalls.CreateEntityWithTransform_Prefab(handle, ref position, ref rotation, ref scale);

            return new Entity(entityHandle);
        }

        public Entity Instantiate(Vec3 position, Vec3 rotation)
        {
            if (handle == ulong.MaxValue)
            {
                throw new InvalidOperationException("Prefab handle is not set.");
            }

            Vec3 scale = Vec3.One;
            ulong entityHandle = InternalCalls.CreateEntityWithTransform_Prefab(handle, ref position, ref rotation, ref scale);

            return new Entity(entityHandle);
        }

        public Entity Instantiate(Vec3 position, Vec3 rotation, Vec3 scale)
        {
            if (handle == ulong.MaxValue)
            {
                throw new InvalidOperationException("Prefab handle is not set.");
            }

            ulong entityHandle = InternalCalls.CreateEntityWithTransform_Prefab(handle, ref position, ref rotation, ref scale);
            return new Entity(entityHandle);
        }
    }

    public class GraphicsShader : Asset
    {
        // Additional methods for GraphicsShader can be added here
    }

    public class Texture2D : Asset
    {
        // Additional methods for Texture can be added here
    }

    public class Font : Asset
    {
        // Additional methods for Material can be added here
    }

    public class Sound : Asset
    {
        // Additional methods for Animation can be added here
    }

    public class StaticMesh : Asset
    {
        // Additional methods for StaticMesh can be added here
    }

    public class TextureCube : Asset
    {
        // Additional methods for TextureCube can be added here
    }

    public class Texture2DArray : Asset
    {
        // Additional methods for Texture2DArray can be added here
    }

    public class SkeletalMesh : Asset
    {
        
    }

    public class SkeletalAnimation : Asset
    {
        // Additional methods for Animation can be added here
    }

    public class Material : Asset
    {
        // Additional methods for Material can be added here
    }

    public class Skeleton : Asset
    {
        // Additional methods for Skeleton can be added here
    }
}