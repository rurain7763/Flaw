namespace Flaw
{
    public class ScriptableComponent
    {
        internal Entity entity;

        public ScriptableComponent()
        {
            entity = new Entity();
        }

        internal ScriptableComponent(uint id)
        {
            entity = new Entity(id);
        }

        public bool HasComponent<T>() where T : ScriptableComponent, new()
        {
            return entity.HasComponent<T>();
        }

        public T? GetComponent<T>() where T : ScriptableComponent, new()
        {
            return entity.GetComponent<T>();
        }

        public Entity FindEntityByName(string name)
        {
            return entity.FindEntityByName(name);
        }
    }
}