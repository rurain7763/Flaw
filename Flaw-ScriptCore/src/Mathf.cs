using System;

namespace Flaw
{
    public static class Mathf
    {
        public const float Epsilon = 1e-6f;
        
        public static bool ApproximatelyEqual(float a, float b, float epsilon = Epsilon)
        {
            return Math.Abs(a - b) < epsilon;
        }
    }
}