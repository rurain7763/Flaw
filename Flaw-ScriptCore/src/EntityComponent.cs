namespace Flaw
{
    public class EntityComponent
    {
        internal Entity entity;

        public Entity Owner
        {
            get { return entity; }
        }

        public EntityComponent()
        {
            entity = new Entity();
        }

        internal EntityComponent(ulong id)
        {
            entity = new Entity(id);
        }

        public bool HasComponent<T>() where T : EntityComponent, new()
        {
            return entity.HasComponent<T>();
        }

        public T GetComponent<T>() where T : EntityComponent, new()
        {
            return entity.GetComponent<T>();
        }

        public Entity FindEntityByName(string name)
        {
            return Entity.FindEntityByName(name);
        }
    }
}