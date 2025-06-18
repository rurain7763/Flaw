namespace Flaw
{
    public class EntityComponent
    {
        internal ulong entityId;

        public Entity Owner
        {
            get { return (Entity)InternalCalls.GetEntity(entityId); }
        }

        public EntityComponent()
        {
            entityId = ulong.MaxValue;
        }

        internal EntityComponent(ulong id)
        {
            entityId = id;
        }

        public bool HasComponent<T>() where T : EntityComponent
        {
            return Owner.HasComponent<T>();
        }

        public bool TryGetComponent<T>(out T component) where T : EntityComponent
        {
            return Owner.TryGetComponent<T>(out component);
        }

        public T GetComponent<T>() where T : EntityComponent
        {
            return Owner.GetComponent<T>();
        }

        public Entity FindEntityByName(string name)
        {
            return Entity.FindEntityByName(name);
        }
    }
}