using System;

namespace Flaw
{
    public readonly struct EntityID
    {
        public readonly ulong id;

        public EntityID(ulong id)
        {
            this.id = id;
        }

        public static EntityID Invalid => new EntityID(ulong.MaxValue);

        public static implicit operator EntityID(ulong id) => new EntityID(id);
        public static implicit operator ulong(EntityID entityId) => entityId.id;
    }

    public class Entity
    {
        internal EntityID id;

        public string Name
        {
            get { return InternalCalls.GetEntityName_Entity(id); }
        }

        public Entity()
        {
            id = EntityID.Invalid;
        }

        public Entity(EntityID id)
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
                entity.id = EntityID.Invalid;
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