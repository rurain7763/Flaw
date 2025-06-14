namespace Flaw
{
    public static class Physics
    {
        public static bool Raycast(Ray ray, out RayHit hit)
        {
            hit = new RayHit();
            return InternalCalls.Raycast_Physics(ref ray, out hit);
        }
    }
}