using CSLauncher;
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;

namespace Interop
{
    class RenderNative
    {
        [DllImport("../Render_Debug_x64.dll")]
        public extern static IntPtr CreateRenderModule();

        [DllImport("../Render_Debug_x64.dll")]
        public extern static void AdaptWindow(IntPtr renderModule, int portType, WindowRuntimeInfo windowInfo);

        [DllImport("../Render_Debug_x64.dll")]
        public extern static void InitialRenderModule(IntPtr renderModule);

        [DllImport("../Render_Debug_x64.dll")]
        public extern static void RenderThroughRenderModule(IntPtr renderModule);

        [DllImport("../Render_Debug_x64.dll")]
        public extern static void TickRenderModule(IntPtr renderModule, IntPtr timer);


        //[DllImport("../Render_Debug_x64.dll")]
        //public extern static void SetGraphicProgram(in GraphicProgram program);
    }
}
