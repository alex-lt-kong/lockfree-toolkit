using Fclp;
using IpcPcQueue.Queue;


namespace IpcPcQueue.Benchmark
{
    internal class AppArgs
    {
        public string Role { get; set; }
        public string QueueName { get; set; }
        public string ProducerIdStr { get; set; }
        public byte ProducerId => byte.Parse(ProducerIdStr);
        public string QueueCapacityStr { get; set; }
        public short QueueCapacity => short.Parse(QueueCapacityStr);
    }

    class Program
    {
        private static Reference<bool> evFlag;

        private static AppArgs? ReadCommandLineArgs(string[] args)
        {
            var parser = new FluentCommandLineParser<AppArgs?>();
            parser.Setup(arg => arg.Role).As('r', "role")
                .WithDescription("Valid values are: 'SpscConsumer', 'MpscConsumer', 'SpscProducer', 'MpscProducer'")
                .Required();
            parser.Setup(arg => arg.QueueName).As('q', "queue-name").WithDescription("Name of the queue, note that producers and consumer must share the same name").Required();
            parser.Setup(arg => arg.ProducerIdStr).As('i', "producer-id").SetDefault("0");
            parser.Setup(arg => arg.QueueCapacityStr).As('c', "queue-capacity").WithDescription("the number of message the queue must be able to hold")
                .SetDefault("100"); // We want a smaller number as the queue is always saturated during test
            parser.SetupHelp("h", "help").Callback(text =>
            {
                Console.WriteLine(text);
                Environment.Exit(0);
            });

            var result = parser.Parse(args);
            if (!result.HasErrors) return parser.Object;
            Console.Error.WriteLine(result.ErrorText);
            return null;
        }

        static unsafe void Main(string[] args)
        {
            var commandLine = ReadCommandLineArgs(args);
            if (commandLine == null) return;
            // const int capacity = 100;
            evFlag = SignalHandler.RegisterSignalHandler(commandLine.Role);
            switch (commandLine.Role)
            {
                case "SpscProducer":
                {
                    var queue = new SpscQueue(commandLine.QueueName, false, commandLine.QueueCapacity,
                        sizeof(MyPayload));
                    queue.Init();
                    RunProducer(queue, commandLine.ProducerId);
                    queue.Dispose(); // Seems somehow C#'s "RAII" is not that robust/deterministic, let's clean it up ourselves
                    break;
                }
                case "MpscProducer":
                {
                    var queue = new MpscQueue(commandLine.QueueName, false, commandLine.QueueCapacity,
                        sizeof(MyPayload));
                    queue.Init();
                    RunProducer(queue, commandLine.ProducerId);
                    queue.Dispose();
                    break;
                }
                case "SpscConsumer":
                {
                    var queue = new SpscQueue(commandLine.QueueName, true, commandLine.QueueCapacity,
                        sizeof(MyPayload));
                    queue.Init();
                    RunConsumer(queue);
                    queue.Dispose();
                    break;
                }
                case "MpscConsumer":
                {
                    var queue = new MpscQueue(commandLine.QueueName, true, commandLine.QueueCapacity,
                        sizeof(MyPayload));
                    queue.Init();
                    RunConsumer(queue);
                    queue.Dispose();
                    break;
                }
                default:
                    Console.WriteLine("Invalid role specified.");
                    return;
            }

            Console.WriteLine($"{commandLine.Role} exited gracefully");
        }

        private static void RunConsumer(IQueue queue)
        {
            Console.WriteLine("Consumer started and initializing the queue...");

            var prevT1 = Utils.GetUnixTimeUs();
            byte[] msgBytes;
            var seqNumByProducers = new long[byte.MaxValue];
            Array.Fill(seqNumByProducers, -1);
            unsafe
            {
                msgBytes = new byte[sizeof(MyPayload)];
            }

            MyPayload payload;
            long msgCount = 0;
            long sampleCount = 0;
            long maxLatencyUs = -1;
            const long stdoutIntervalSec = 5;
            //var queue = new T(queueName);
            Console.WriteLine("Waiting for messages...");
            while (!evFlag.Value)
            {
                //Thread.Sleep(1);
                //Console.WriteLine($"Before Dequeue(), head: {queue.GetHeadIndex()}, tail: {queue.Tail()}, UsedSpace: {queue.GetUsedBytes()}");
                if (queue.Dequeue(ref msgBytes) <= 0)
                {
                    continue;
                }

                //Console.WriteLine($"After  Dequeue(), head: {queue.GetHeadIndex()}, tail: {queue.Tail()}, UsedSpace: {queue.GetUsedBytes()}");
                unsafe
                {
                    fixed (byte* bytePtr = msgBytes)
                    {
                        payload = *(MyPayload*)bytePtr;
                    }
                }

                var t1 = Utils.GetUnixTimeUs();
                ++msgCount;

                if (payload.SeqNum != seqNumByProducers[payload.ProducerId] + 1)
                {
                    Console.Error.WriteLine(
                        $"producerId: {payload.ProducerId}, prevSeqNum: {seqNumByProducers[payload.ProducerId]}, payload.SeqNum: {payload.SeqNum}, diff:{payload.SeqNum - seqNumByProducers[payload.ProducerId]}");
                }

                seqNumByProducers[payload.ProducerId] = payload.SeqNum;
                if (!(t1 - prevT1 > stdoutIntervalSec * 1_000_000) /*&& queue.GetUsedBytes() > 0*/) continue;

                ++sampleCount;
                var latencyUs = t1 - payload.UnixTimeTs;
                if (maxLatencyUs < latencyUs && sampleCount > 1)
                    maxLatencyUs = latencyUs;
                Console.WriteLine(
                    $"SeqNum: {payload.SeqNum}, ProducerId: {payload.ProducerId}, t0: {payload.UnixTimeTs}, latencyUs: {latencyUs}, maxLatencyUs (percentile:{(1 - 1.0 / sampleCount) * 100.0:n2}): {maxLatencyUs}, msgCount/sec: {msgCount / stdoutIntervalSec:n0}");
                prevT1 = t1;
                msgCount = 0;
            }
        }

        static void RunProducer(IQueue queue, byte producerId)
        {
            var byteArrayPool = new byte[128][];
            for (int i = 0; i < byteArrayPool.Length; i++)
            {
                unsafe
                {
                    byteArrayPool[i] = new byte[sizeof(MyPayload)];
                }
            }

            //var queue = new MpscQueue(queueName);
            Console.WriteLine("Producer started.");
            long msgCount = 0;
            long idx = 0;
            MyPayload payload;
            payload.ProducerId = producerId;
            while (!evFlag.Value)
            {
                payload.UnixTimeTs = Utils.GetUnixTimeUs();
                payload.SeqNum = msgCount;
                unsafe
                {
                    fixed (byte* bytePtr = byteArrayPool[idx])
                    {
                        var structPtr = (byte*)&payload;
                        for (int i = 0; i < sizeof(MyPayload); i++)
                        {
                            bytePtr[i] = structPtr[i];
                        }
                    }
                }

                bool success = queue.Enqueue(byteArrayPool[idx]);
                //  Thread.Sleep(1000);
                if (!success)
                {
                    Console.WriteLine($"Enqueue() failed (msgCount: {msgCount})");
                    //Thread.Sleep(1);
                }
                else
                {
                    msgCount++;
                    //    Console.WriteLine($"Enqueue()ed (head: {queue.GetHeadIndex()}, tail: {queue.Tail()}, msgCount: {msgCount}, UsedSpace: {queue.GetUsedBytes()})");
                    idx = msgCount % 128;
                }
            }
        }
    }
}