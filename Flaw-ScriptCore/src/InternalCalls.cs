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
        internal extern static bool IsEngineComponent(Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool HasComponent(ulong id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool IsComponentInstanceExists(ulong id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static object GetComponentInstance(ulong id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void DestroyEntity(ulong id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong FindEntityByName(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong CreateEntity_Prefab(ulong handle);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static ulong CreateEntityWithTransform_Prefab(ulong handle, ref Vec3 position, ref Vec3 rotation, ref Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetPosition_Transform(ulong id, out Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetPosition_Transform(ulong id, ref Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetRotation_Transform(ulong id, out Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetRotation_Transform(ulong id, ref Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetScale_Transform(ulong id, out Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetScale_Transform(ulong id, ref Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetBodyType_RigidBody2D(ulong id, out int type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetBodyType_RigidBody2D(ulong id, int type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetLinearVelocity_RigidBody2D(ulong id, out Vec2 velocity);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void ScreenToWorld_Camera(ulong id, ref Vec2 screenPosition, out Vec3 worldPosition);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyDown_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyUp_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKey_Input(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetMousePosition_Input(ref float x, ref float y);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool Raycast_Physics(ref Ray ray, out RayHit hit);
    }
}
