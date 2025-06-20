namespace Flaw
{
    public struct ContactPoint
    {
        public Vec3 position;
        public Vec3 normal;
        public Vec3 impulse;

        internal ContactPoint(Vec3 position, Vec3 normal, Vec3 impulse)
        {
            this.position = position;
            this.normal = normal;
            this.impulse = impulse;
        }
    }

    public class CollisionInfo
    {
        public ColliderComponent collider; // Collider that was hit
        public ColliderComponent otherCollider; // Other collider involved in the collision

        public ContactPoint[] contactPoints; // Array of contact points in the collision

        internal CollisionInfo(ColliderComponent collider, ColliderComponent otherCollider, ContactPoint[] contactPoints)
        {
            this.collider = collider;
            this.otherCollider = otherCollider;
            this.contactPoints = contactPoints;
        }
    }

    public class TriggerInfo
    {
        public ColliderComponent collider; // Collider that triggered the event
        public ColliderComponent otherCollider; // Other collider involved in the trigger event
        
        internal TriggerInfo(ColliderComponent collider, ColliderComponent otherCollider)
        {
            this.collider = collider;
            this.otherCollider = otherCollider;
        }
    }

    public static class Physics
    {
        public static bool Raycast(Ray ray, out RayHit hit)
        {
            hit = new RayHit();
            return InternalCalls.Raycast_Physics(ref ray, out hit);
        }
    }
}