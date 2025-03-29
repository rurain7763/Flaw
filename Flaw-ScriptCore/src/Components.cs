namespace Flaw {
    public class TransformComponent : EntityComponent
    {
        public Vec3 position
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

        public Vec3 rotation
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

        public Vec3 scale
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
        
    }
}