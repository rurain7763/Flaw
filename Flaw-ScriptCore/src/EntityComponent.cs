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

        public bool HasComponent<T>() where T : EntityComponent, new()
        {
            return Owner.HasComponent<T>();
        }

        public T GetComponent<T>() where T : EntityComponent, new()
        {
            return Owner.GetComponent<T>();
        }

        public Entity FindEntityByName(string name)
        {
            return Entity.FindEntityByName(name);
        }
    }
}