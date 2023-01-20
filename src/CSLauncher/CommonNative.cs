using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;


namespace Interop
{
    class CommonNative
    {
        [DllImport("../Common_Debug_x64.dll")]
        public extern static IntPtr CreateTimer();

        [DllImport("../Common_Debug_x64.dll")]
        public extern static void TimerOnStartNewFrame(IntPtr timer); 
    }
}