namespace Flaw
{
    public static class Input
    {
        public static bool GetKeyDown(KeyCode keyCode)
        {
            return InternalCalls.GetKeyDown_Input(keyCode);
        }

        public static bool GetKeyUp(KeyCode keyCode)
        {
            return InternalCalls.GetKeyUp_Input(keyCode);
        }

        public static bool GetKey(KeyCode keyCode)
        {
            return InternalCalls.GetKey_Input(keyCode);
        }
    }
}