using System;
using System.Runtime.InteropServices;

namespace Interop
{
    class WinGuiNative
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct Message
        {
            public UInt64 mWindowHandle;
            public UInt64 mMessage;
            public UInt64 mWParam;
            public UInt64 mLParam;
        }

        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static IntPtr CreateWinGuiSystem();

        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static void UpdateWinGuiSystem(IntPtr system);

        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static void FlushMessages(IntPtr system);

        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static bool DequeueMessage(IntPtr system, out Message message);
    }
}
