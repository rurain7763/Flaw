using System;

namespace Flaw
{
    public class Asset
    {
        internal ulong handle;

        public Asset()
        {
            handle = ulong.MaxValue;
        }

        public Asset(ulong handle)
        {
            this.handle = handle;
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
}