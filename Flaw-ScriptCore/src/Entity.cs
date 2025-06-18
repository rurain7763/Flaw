using System;

namespace Flaw
{
    public class Entity
    {
        internal ulong id;

        public Entity()
        {
            id = ulong.MaxValue;
        }

        public Entity(ulong id)
        {
            this.id = id;
        }

        public bool HasComponent<T>() where T : EntityComponent
        {
            Type type = typeof(T);
            return InternalCalls.HasComponent(id, type);
        }

        public bool TryGetComponent<T>(out T component) where T : EntityComponent
        {
            Type type = typeof(T);
            if (InternalCalls.HasComponent(id, type))
            {
                component = (T)InternalCalls.GetComponentInstance(id, type);
                return true;
            }
            component = null;
            return false;
        }

        public T GetComponent<T>() where T : EntityComponent
        {
            Type type = typeof(T);
            return (T)InternalCalls.GetComponentInstance(id, type);
        }

        public void Destroy()
        {
            Destroy(this);
        }

        public static void Destroy(Entity entity)
        {
            if (entity.id != ulong.MaxValue)
            {
                InternalCalls.DestroyEntity(entity.id);
                entity.id = ulong.MaxValue; // Mark as destroyed
            }
        }

        public static Entity FindEntityByName(string name)
        {
            ulong id = InternalCalls.FindEntityByName(name);
            if (id != ulong.MaxValue)
            {
                return new Entity(id);
            }

            return new Entity();
        }
    }
}