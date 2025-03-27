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
        internal extern static bool IsEngineComponent(Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool HasComponent(uint id, Type type);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool IsComponentInstanceExists(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static object GetComponentInstance(uint id);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static uint FindEntityByName(string name);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetPosition_Transform(uint id, out Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetPosition_Transform(uint id, ref Vec3 position);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetRotation_Transform(uint id, out Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetRotation_Transform(uint id, ref Vec3 rotation);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void GetScale_Transform(uint id, out Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static void SetScale_Transform(uint id, ref Vec3 scale);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyDown(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKeyUp(KeyCode keyCode);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal extern static bool GetKey(KeyCode keyCode);
    }
}
