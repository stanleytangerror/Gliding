using System;
using System.Runtime.InteropServices;

namespace Interop
{
    class WinGuiNative
    {
        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static IntPtr CreateWinGuiSystem();

        [DllImport("../WinGui_Debug_x64.dll")]
        public extern static void UpdateWinGuiSystem(IntPtr system);
    }
}
