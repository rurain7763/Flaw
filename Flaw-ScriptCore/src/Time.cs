namespace Flaw
{
    public static class Time
    {
        public static float DeltaTime
        {
            get
            {
                return InternalCalls.GetDeltaTime();
            }
        }
    }
}