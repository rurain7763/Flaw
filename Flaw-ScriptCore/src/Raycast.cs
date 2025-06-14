namespace Flaw
{
    public struct Ray
    {
        public Vec3 origin;
        public Vec3 direction;
        public float length;
    }

    public struct RayHit
    {
        public Vec3 position;
        public Vec3 normal;
        public float distance;
    }
}