namespace Flaw {
    public class TransformComponent : EntityComponent
    {
        public Vec3 Position
        {
            get
            {
                InternalCalls.GetPosition_Transform(entity.id, out Vec3 position);
                return position;
            }
            set
            {
                InternalCalls.SetPosition_Transform(entity.id, ref value);
            }
        }

        public Vec3 Rotation
        {
            get
            {
                InternalCalls.GetRotation_Transform(entity.id, out Vec3 rotation);
                return rotation;
            }
            set
            {
                InternalCalls.SetRotation_Transform(entity.id, ref value);
            }
        }

        public Vec3 Scale
        {
            get
            {
                InternalCalls.GetScale_Transform(entity.id, out Vec3 scale);
                return scale;
            }
            set
            {
                InternalCalls.SetRotation_Transform(entity.id, ref value);
            }
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
                InternalCalls.GetBodyType_RigidBody2D(entity.id, out int type);
                return (BodyType)type;
            }
            set
            {
                InternalCalls.SetBodyType_RigidBody2D(entity.id, (int)value);
            }
        }

        public Vec2 LinearVelocity
        {
            get
            {
                InternalCalls.GetLinearVelocity_RigidBody2D(entity.id, out Vec2 velocity);
                return velocity;
            }
        }
    }
}