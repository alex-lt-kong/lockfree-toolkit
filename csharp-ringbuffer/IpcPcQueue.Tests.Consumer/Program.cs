using IpcPcQueue.Benchmark;
using IpcPcQueue.Queue;

namespace IpcPcQueue.Tests.Consumer
{
    internal abstract class Program
    {
        private static Reference<bool> evFlag;


        static void Main(string[] args)
        {
            evFlag = SignalHandler.RegisterSignalHandler("IpcPcQueue.Tests.Consumer");
            Dictionary<string, string> argsToTest = new Dictionary<string, string>()
            {
                {"0", "SpscShouldNotMissMsgs"}
            };
            if (args.Length != 1 || !argsToTest.ContainsKey(args[0]))
            {
                Console.Error.WriteLine("Invalid arguments");
                return;
            }

            switch (args[0])
            {
                case "0":
                {
                    ConsumerSpscShouldNotMissMsgs();
                    break;
                }
                default:
                    Console.WriteLine("Invalid role specified.");
                    return;
            }

            Console.WriteLine($"{argsToTest[args[0]]} exited gracefully");
        }

        private static unsafe void ConsumerSpscShouldNotMissMsgs()
        {
            var q = new SpscQueue("SpscShouldNotMissMsgs");
            q.Init();
            var msgBytes = new byte[sizeof(long)];
            long expectedSeqNum = -1;
            while (!evFlag.Value)
            {
                if (q.Dequeue(ref msgBytes) != 8)
                    continue;
                long receivedSeqNum = -1;
                fixed (byte* ptr = &msgBytes[0])
                    receivedSeqNum = *(long*)ptr;
                if (receivedSeqNum != expectedSeqNum + 1)
                    throw new ApplicationException($"receivedSeqNum({receivedSeqNum}) != expectedSeqNum({expectedSeqNum}) + 1");
                Console.WriteLine($"Good, receivedSeqNum({receivedSeqNum}) == expectedSeqNum ({expectedSeqNum}) + 1");
                expectedSeqNum = receivedSeqNum;

            }
        }
    }
}