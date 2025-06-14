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

        public static bool operator ==(EntityComponent left, EntityComponent right)
        {
            if (ReferenceEquals(left, right)) return true;
            if (left is null || right is null) return false;
            return left.entity.id == right.entity.id;
        }

        public static bool operator !=(EntityComponent left, EntityComponent right)
        {
            return !(left == right);
        }

        public override bool Equals(object obj)
        {
            if (obj is EntityComponent other)
            {
                return this == other;
            }
            return false;
        }

        public override int GetHashCode()
        {
            return entity.id.GetHashCode();
        }
    }
}