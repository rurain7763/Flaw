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

    public class ColliderComponent : EntityComponent
    {
        // Collider specific properties and methods can be added here
    }

    public class BoxColliderComponent : ColliderComponent
    {
        public Vec3 Offset
        {
            get
            {
                return Vec3.Zero; // Placeholder, actual implementation should retrieve offset from the internal call
            }
            set
            {
                // Placeholder, actual implementation should set offset using an internal call
            }
        }

        public Vec3 Size 
        { 
            get
            {
                return Vec3.Zero; // Placeholder, actual implementation should retrieve size from the internal call
            }
            set
            {
                // Placeholder, actual implementation should set size using an internal call
            }
        }
    }

    public class SphereColliderComponent : ColliderComponent
    {
        public Vec3 Offset
        {
            get
            {
                return Vec3.Zero; // Placeholder, actual implementation should retrieve offset from the internal call
            }
            set
            {
                // Placeholder, actual implementation should set offset using an internal call
            }
        }
        public float Radius
        {
            get
            {
                return 0f; // Placeholder, actual implementation should retrieve radius from the internal call
            }
            set
            {
                // Placeholder, actual implementation should set radius using an internal call
            }
        }
    }

    public class MeshColliderComponent : ColliderComponent
    {
        // MeshCollider specific properties and methods can be added here
    }

    public class AnimatorComponent : EntityComponent
    {
        public void PlayState(int stateIndex)
        {
            InternalCalls.PlayState_Animator(entityId, stateIndex);
        }
    }

    public class SkeletalMeshComponent : EntityComponent
    {
        public void AttachEntityToSocket(Entity entity, string socketName)
        {
            InternalCalls.AttachEntityToSocket_SkeletalMesh(entityId, entity.id, socketName);
        }
    }
}