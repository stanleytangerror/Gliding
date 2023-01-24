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


        private Thread mGuiThead;
        private Thread mLogicThread;

        IntPtr mRenderModule;
        IntPtr mTimer;
        WindowId mMainWindowId;
        WindowId mDebugWindowId;

        void Initial()
        {
            mGuiThead = new Thread(GuiThread);

            mLogicThread = new Thread(LogicThread);

            mTimer = CommonNative.CreateTimer();

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
            mLogicThread.Join();
        }

        void TriggerQuitting()
        {
            mProgramStatus = Status.Quitting;
        }

        void GuiThread()
        {
            var guiSystem = WinGuiNative.CreateWinGuiSystem();

            mMainWindowId = WinGuiNative.CreateNewGuiWindow(guiSystem, "Main", new Vec2i { x = 1600, y = 400 });
            mDebugWindowId = WinGuiNative.CreateNewGuiWindow(guiSystem, "Debug", new Vec2i { x = 1200, y = 600 });

            GuiSystem = guiSystem;

            while (mProgramStatus != Status.Quitting)
            {
                WinGuiNative.FlushMessages(guiSystem);
                while (WinGuiNative.DequeueMessage(guiSystem, out var message))
                {
                    // message handling
                }
            }
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

            mRenderModule = RenderNative.CreateRenderModule();
            ImGuiIntegrationNative.Initial();

            var guiSystem = GuiSystem;
            if (WinGuiNative.TryGetGuiWindowInfo(guiSystem, mMainWindowId, out var info1))
            {
                RenderNative.AdaptWindow(mRenderModule, 0, info1);
            }
            if (WinGuiNative.TryGetGuiWindowInfo(guiSystem, mDebugWindowId, out var info2))
            {
                RenderNative.AdaptWindow(mRenderModule, 1, info2);
            }

            RenderNative.InitialRenderModule(mRenderModule);
            while (mProgramStatus != Status.Quitting)
            {
                CommonNative.TimerOnStartNewFrame(mTimer);
                RenderNative.TickRenderModule(mRenderModule, mTimer);
                RenderNative.RenderThroughRenderModule(mRenderModule);
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
