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
    }
}