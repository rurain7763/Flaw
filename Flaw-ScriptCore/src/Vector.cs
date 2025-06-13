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

        public static readonly Vec3 Forward = new Vec3(0, 0, 1);
        public static readonly Vec3 Up = new Vec3(0, 1, 0);
        public static readonly Vec3 Right = new Vec3(1, 0, 0);
        public static readonly Vec3 Zero = new Vec3(0, 0, 0);
        public static readonly Vec3 One = new Vec3(1, 1, 1);

        public float Length()
        {
            return (float)Math.Sqrt(x * x + y * y + z * z);
        }

        public float SqrLength()
        {
            return x * x + y * y + z * z;
        }

        public void Normalize()
        {
            float length = Length();
            if (length == 0) return; // Avoid division by zero
            x /= length;
            y /= length;
            z /= length;
        }

        public Vec3 Normalized()
        {
            float length = Length();
            if (length == 0) return new Vec3(0, 0, 0); // Avoid division by zero
            return new Vec3(x / length, y / length, z / length);
        }

        public static float Dot(Vec3 a, Vec3 b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        public static Vec3 operator +(Vec3 a, Vec3 b)
        {
            return new Vec3(a.x + b.x, a.y + b.y, a.z + b.z);
        }

        public static Vec3 operator -(Vec3 a, Vec3 b)
        {
            return new Vec3(a.x - b.x, a.y - b.y, a.z - b.z);
        }
    }
}