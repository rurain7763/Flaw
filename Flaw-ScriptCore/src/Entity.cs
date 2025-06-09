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

        public bool HasComponent<T>() where T : EntityComponent, new()
        {
            Type type = typeof(T);

            if (InternalCalls.IsEngineComponent(type))
            {
                return InternalCalls.HasComponent(id, type);
            }
            else
            {
                return InternalCalls.IsComponentInstanceExists(id);
            }
        }

        public T GetComponent<T>() where T : EntityComponent, new()
        {
            Type type = typeof(T);

            if (InternalCalls.IsEngineComponent(type))
            {
                if (InternalCalls.HasComponent(id, type))
                {
                    T t = new T();
                    t.entity = this;
                    return t;
                }
            }
            else if (InternalCalls.IsComponentInstanceExists(id))
            {
                return (T)InternalCalls.GetComponentInstance(id);
            }
            
            return null;
        }

        public Entity FindEntityByName(string name)
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