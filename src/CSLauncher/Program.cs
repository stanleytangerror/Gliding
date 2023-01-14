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
        private Thread mGuiThead;

        void Initial()
        {
            mGuiThead = new Thread(GuiThread);

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

        static void Main(string[] args)
        {
            var program = new Program();
            program.Initial();
            program.Run();
            program.Quit();          
        }
    }
}
