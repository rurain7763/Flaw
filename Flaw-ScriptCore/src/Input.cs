namespace Flaw
{
    public static class Input
    {
        public static bool GetKeyDown(KeyCode keyCode)
        {
            return InternalCalls.GetKeyDown(keyCode);
        }

        public static bool GetKeyUp(KeyCode keyCode)
        {
            return InternalCalls.GetKeyUp(keyCode);
        }

        public static bool GetKey(KeyCode keyCode)
        {
            return InternalCalls.GetKey(keyCode);
        }
    }
}