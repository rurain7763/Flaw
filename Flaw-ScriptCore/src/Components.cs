namespace Flaw {
    public class TransformComponent : EntityComponent
    {
        public Vec3 Position
        {
            get
            {
                InternalCalls.GetPosition_Transform(entityId, out Vec3 position);
                return position;
            }
            set
            {
                InternalCalls.SetPosition_Transform(entityId, ref value);
            }
        }

        public Vec3 Rotation
        {
            get
            {
                InternalCalls.GetRotation_Transform(entityId, out Vec3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.SetRotation_Transform(entityId, ref value);
            }
        }

        public Vec3 Scale
        {
            get
            {
                InternalCalls.GetScale_Transform(entityId, out Vec3 scale);
                return scale;
            }
            set
            {
                InternalCalls.SetRotation_Transform(entityId, ref value);
            }
        }

        public Vec3 Forward
        {
            get
            {
                InternalCalls.GetForward_Transform(entityId, out Vec3 forward);
                return forward;
            }
        }

        public void LookAt(Vec3 target)
        {
            Vec3 dir = (target - Position).Normalized();

            float pitch = (float)System.Math.Asin(-dir.y);
            float yaw = (float)System.Math.Atan2(dir.x, dir.z);

            Rotation = new Vec3(pitch, yaw, 0);
        }
    }

    public class Rigidbody2DComponent : EntityComponent
    {
        public enum BodyType
        {
            Static,
            Dynamic,
            Kinematic
        }

        public BodyType Type
        {
            get
            {
                InternalCalls.GetBodyType_RigidBody2D(entityId, out int type);
                return (BodyType)type;
            }
            set
            {
                InternalCalls.SetBodyType_RigidBody2D(entityId, (int)value);
            }
        }

        public Vec2 LinearVelocity
        {
            get
            {
                InternalCalls.GetLinearVelocity_RigidBody2D(entityId, out Vec2 velocity);
                return velocity;
            }
        }
    }

    public class CameraComponent : EntityComponent
    {
        public Vec3 ScreenToWorld(Vec2 screenPosition)
        {
            Vec3 worldPosition = new Vec3();
            InternalCalls.ScreenToWorld_Camera(entityId, ref screenPosition, out worldPosition);
            return worldPosition;
        }
    }
}