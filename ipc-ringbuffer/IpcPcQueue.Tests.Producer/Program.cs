using IpcPcQueue.Benchmark;
using IpcPcQueue.Queue;

namespace IpcPcQueue.Tests.Producer
{
    internal static class Program
    {
        private static Reference<bool> evFlag;


        static void Main(string[] args)
        {
            evFlag = SignalHandler.RegisterSignalHandler("IpcPcQueue.Tests.Producer");
            Dictionary<string, string> argsToTest = new Dictionary<string, string>()
            {
                {"0", "SpscShouldNotMissMsgs"}
            };

            if (args.Length < 1 || !argsToTest.ContainsKey(args[0]))
            {
                Console.Error.WriteLine("Invalid arguments");
                return;
            }
            
            switch (args[0])
            {
                case "0":
                {
                    ProducerSpscShouldNotMissMsgs();
                    break;
                }
                default:
                    Console.WriteLine("Invalid role specified.");
                    return;
            }

            Console.WriteLine($"{argsToTest[args[0]]} exited gracefully");
        }

        private static unsafe void ProducerSpscShouldNotMissMsgs()
        {
            var q = new SpscQueue("SpscShouldNotMissMsgs");
            var rnd = new Random();
            q.Init();
            var msgBytes = new byte[sizeof(long)];
            long seqNum = 0;
            
            while (!evFlag.Value)
            {
                fixed (byte* ptr = &msgBytes[0])
                    *(long*)ptr = seqNum;
                if (q.Enqueue(msgBytes))
                    ++seqNum;
                else
                {
                    Console.WriteLine($"queue is full, current seqNum=={seqNum}, will sleep and retry");
                    var sleepTime = rnd.Next(10000);
                    for (int i = 0; i < sleepTime; ++i)
                    {
                        // Thread.Sleep(1) sleeps too long...
                    }
                }
            }
        }
    }
}