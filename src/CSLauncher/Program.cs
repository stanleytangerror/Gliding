using Interop;
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace CSLauncher
{
    class Program
    {
        enum Status
        {
            Initializing, Running, Quitting,
        }

        private Status mProgramStatus = Status.Initializing;
        
        private IntPtr GuiSystem
        {
            get { return (IntPtr) Interlocked.Read(ref _mGuiSystem); }
            set { Interlocked.Exchange(ref _mGuiSystem, (long) value); }
        }
        private long _mGuiSystem;


        IntPtr mRenderModule;
        IntPtr mTimer;
        private Thread mGuiThead;
        private Thread mLogicThread;

        WindowId mMainWindowId;
        WindowId mDebugWindowId;

        void Initial()
        {
            mGuiThead = new Thread(GuiThread);

            mLogicThread = new Thread(LogicThread);

            mTimer = Interop.CommonNative.CreateTimer();

            mProgramStatus = Status.Running;
        }

        void Run()
        {
            mGuiThead.Start();
            mLogicThread.Start();
        }

        void Quit()
        {
            mGuiThead.Join();
        }

        void GuiThread()
        {
            var guiSystem = Interop.WinGuiNative.CreateWinGuiSystem();

            mMainWindowId = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Main", new Interop.Vec2i { x = 1600, y = 800 });
            mDebugWindowId = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Debug", new Interop.Vec2i { x = 1200, y = 600 });

            GuiSystem = guiSystem;

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

        async Task WaitForGuiSystemAsync()
        {
            while (GuiSystem == default(IntPtr)) 
            { 
                await Task.Delay(10);
            }
        }

        void LogicThread()
        {
            WaitForGuiSystemAsync().Wait();

            mRenderModule = Interop.RenderNative.CreateRenderModule();
            Interop.ImGuiIntegrationNative.Initial();

            var guiSystem = GuiSystem;
            if (Interop.WinGuiNative.TryGetGuiWindowInfo(guiSystem, mMainWindowId, out var info1))
            {
                Interop.RenderNative.AdaptWindow(mRenderModule, 0, info1);
            }
            if (Interop.WinGuiNative.TryGetGuiWindowInfo(guiSystem, mDebugWindowId, out var info2))
            {
                Interop.RenderNative.AdaptWindow(mRenderModule, 1, info2);
            }

            Interop.RenderNative.InitialRenderModule(mRenderModule);
            while (true)
            {
                Interop.CommonNative.TimerOnStartNewFrame(mTimer);
                Interop.RenderNative.TickRenderModule(mRenderModule, mTimer);
                Interop.RenderNative.RenderThroughRenderModule(mRenderModule);
            }
        }

        static void SetupEnvironment()
        {
            var dir = Directory.GetCurrentDirectory();
            var appDir = dir + @"\..\..\..\..\";
            Directory.SetCurrentDirectory(appDir);
        }

        static void Main(string[] args)
        {
            SetupEnvironment();
            
            var program = new Program();
            program.Initial();
            program.Run();
            program.Quit();          
        }
    }
}
