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
        private Thread mWindowThread;

        void Initial()
        {
            mWindowThread = new Thread(WindowThread);

            mProgramStatus = Status.Running;
        }

        void Run()
        {
            mWindowThread.Start();
        }

        void Quit()
        {
            mWindowThread.Join();
        }

        void WindowThread()
        {
            var guiSystem = Interop.WinGuiNative.CreateWinGuiSystem();

            Interop.WinGuiNative.UpdateWinGuiSystem(guiSystem);

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
