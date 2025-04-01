using System;

namespace Flaw
{
    public struct Vec2
    {
        public float x, y;

        public Vec2(float xy)
        {
            x = y = xy;
        }

        public Vec2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }

        public float Length()
        {
            return (float)Math.Sqrt(x * x + y * y);
        }

        public float SqrLength()
        {
            return x * x + y * y;
        }
    }

    public struct Vec3
    {
        public float x, y, z;

        public Vec2 xy
        {
            get { return new Vec2 { x = x, y = y }; }
            set { x = value.x; y = value.y; }
        }

        public Vec3(float xyz)
        {
            x = y = z = xyz;
        }

        public Vec3(Vec2 xy, float z)
        {
            x = xy.x;
            y = xy.y;
            this.z = z;
        }

        public Vec3(float x, float y, float z)
        {
            this.x = x;
            this.y = y;
            this.z = z;
        }
    }
}