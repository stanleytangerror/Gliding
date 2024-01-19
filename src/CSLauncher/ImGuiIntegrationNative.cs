using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Interop
{
    class ImGuiIntegrationNative
    {
        [DllImport("../ImGuiIntegration_Debug_x64.dll")]
        public extern static IntPtr Initial();
    }
}
