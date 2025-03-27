namespace Flaw
{
    public static class Log
    {
        public static void Info(string message)
        {
            InternalCalls.LogInfo(message);
        }
    }
}