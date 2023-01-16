using System;
using System.Threading;

namespace CSLauncher
{
    class Program
    {
        enum Status
        {
            Initializing, Running, Quitting,
        }

        private Status mProgramStatus = Status.Initializing;
        IntPtr mGuiSystem;
        IntPtr mRenderModule;
        private Thread mGuiThead;
        private Thread mLogicThread;

        void Initial()
        {
            mGuiThead = new Thread(GuiThread);

            mLogicThread = new Thread(LogicThread);

            mProgramStatus = Status.Running;
        }

        void Run()
        {
            mGuiThead.Start();
        }

        void Quit()
        {
            mGuiThead.Join();
        }

        void GuiThread()
        {
            var guiSystem = Interop.WinGuiNative.CreateWinGuiSystem();

            Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test1", new Interop.Vec2i { x = 1600, y = 800 });
            Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test2", new Interop.Vec2i { x = 1200, y = 600 });
            Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test3", new Interop.Vec2i { x = 900, y = 400 });
            Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test4", new Interop.Vec2i { x = 600, y = 200 });

            while (true)
            {
                Interop.WinGuiNative.FlushMessages(guiSystem);
                while (Interop.WinGuiNative.DequeueMessage(guiSystem, out var message)) 
                {
                    // message handling
                }
            }

            mProgramStatus = Status.Quitting;
        }

        void LogicThread()
        {
            //mRenderModule = Interop.RenderNative.CreateRenderModule();

            //Interop.RenderNative.AdaptWindow(mRenderModule, new Wind)
        }

        static void Main(string[] args)
        {
            var program = new Program();
            program.Initial();
            program.Run();
            program.Quit();          
        }
    }
}
