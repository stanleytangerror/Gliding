using System;
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
            mLogicThread.Start();
        }

        void Quit()
        {
            mGuiThead.Join();
        }

        void GuiThread()
        {
            var guiSystem = Interop.WinGuiNative.CreateWinGuiSystem();
            GuiSystem = guiSystem;

            var id1 = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test1", new Interop.Vec2i { x = 1600, y = 800 });
            var id2 = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test2", new Interop.Vec2i { x = 1200, y = 600 });
            var id3 = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test3", new Interop.Vec2i { x = 900, y = 400 });
            var id4 = Interop.WinGuiNative.CreateNewGuiWindow(guiSystem, "Test4", new Interop.Vec2i { x = 600, y = 200 });

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

            var guiSystem = GuiSystem;
            if (Interop.WinGuiNative.TryGetGuiWindowInfo(guiSystem, new Interop.WindowId { mId = 1 }, out var info))
            {
                Console.WriteLine(info.mNativeHandle.ToString());
            }
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
