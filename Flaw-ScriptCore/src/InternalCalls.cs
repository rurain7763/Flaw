using System;
using System.Runtime.CompilerServices;

namespace Flaw
{
    public static class InternalCalls
    {
        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void LogInfo(string message);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static float GetDeltaTime();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static float GetTimeSinceStart();

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static object GetEntity(EntityID id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool HasComponent(EntityID id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static object GetComponentInstance(EntityID id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void DestroyEntity(EntityID id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong FindEntityByName(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong CreateEntity_Prefab(AssetHandle handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong CreateEntityWithTransform_Prefab(AssetHandle handle, ref Vec3 position, ref Vec3 rotation, ref Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static string GetEntityName_Entity(EntityID id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetPosition_Transform(EntityID id, out Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetPosition_Transform(EntityID id, ref Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetRotation_Transform(EntityID id, out Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetRotation_Transform(EntityID id, ref Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetScale_Transform(EntityID id, out Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetScale_Transform(EntityID id, ref Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetForward_Transform(EntityID id, out Vec3 forward);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetBodyType_RigidBody2D(EntityID id, out int type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetBodyType_RigidBody2D(EntityID id, int type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetLinearVelocity_RigidBody2D(EntityID id, out Vec2 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void ScreenToWorld_Camera(EntityID id, ref Vec2 screenPosition, out Vec3 worldPosition);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyDown_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyUp_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKey_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetMousePosition_Input(ref float x, ref float y);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetMouseButtonDown_Input(MouseButton button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetMouseButtonUp_Input(MouseButton button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetMouseButton_Input(MouseButton button);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool Raycast_Physics(ref Ray ray, out RayHit hit);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void PlayState_Animator(EntityID id, int stateIndex);
    }
}
