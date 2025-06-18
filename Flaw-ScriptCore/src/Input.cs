namespace Flaw
{
    public static class Input
    {
        public static Vec2 MousePosition
        {
            get
            {
                Vec2 position = new Vec2();
                InternalCalls.GetMousePosition_Input(ref position.x, ref position.y);
                return position;
            }
        }

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

        public static bool GetMouseButtonDown(MouseButton button)
        {
            return InternalCalls.GetMouseButtonDown_Input(button);
        }

        public static bool GetMouseButtonUp(MouseButton button)
        {
            return InternalCalls.GetMouseButtonUp_Input(button);
        }

        public static bool GetMouseButton(MouseButton button)
        {
            return InternalCalls.GetMouseButton_Input(button);
        }
    }
}