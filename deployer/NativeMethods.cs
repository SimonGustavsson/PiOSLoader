using System.Runtime.InteropServices;

namespace PiOSDeployer
{
    public delegate bool HandlerRoutine(CtrlTypes CtrlType);

    public enum CtrlTypes
    {
        CTRL_C_EVENT = 0,
        CTRL_BREAK_EVENT,
        CTRL_CLOSE_EVENT,
        CTRL_LOGOFF_EVENT = 5,
        CTRL_SHUTDOWN_EVENT
    }

    internal static class NativeMethods
    {
        /// <summary>
        /// Adds or removes an application-defined HandlerRoutine function from the list of handler functions for the calling process.
        /// </summary>
        /// <param name="Handler">A pointer to the application-defined HandlerRoutine function to be added or removed. This parameter can be NULL.</param>
        /// <param name="Add">If this parameter is TRUE, the handler is added; if it is FALSE, the handler is removed.</param>
        /// <returns>If the function succeeds, the return value is nonzero.</returns>
        [DllImport("Kernel32")]
        public static extern bool SetConsoleCtrlHandler(HandlerRoutine Handler, bool Add);
    }
}