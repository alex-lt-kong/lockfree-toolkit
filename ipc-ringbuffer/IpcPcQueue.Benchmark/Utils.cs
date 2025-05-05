using System.Runtime.InteropServices;

namespace IpcPcQueue.Benchmark;

public class Reference<T>
{
    public T Value { get; set; }

    public Reference(T initialValue = default)
    {
        Value = initialValue;
    }
}

[StructLayout(LayoutKind.Sequential)]
public struct MyPayload
{
    public Int64 UnixTimeTs;
    public Int64 SeqNum;
    public byte ProducerId; // byte is a one-byte long int. Meaning that we allow 256 producers at most
}

public static class Utils
{
    public static long GetUnixTimeUs()
    {
        DateTimeOffset now = DateTimeOffset.UtcNow;
        TimeSpan timeSinceEpoch = now - DateTimeOffset.UnixEpoch;
        return timeSinceEpoch.Ticks / (TimeSpan.TicksPerMillisecond / 1000);
    }
}

public abstract class SignalHandler
{
    private static readonly Reference<bool> EvFlag  = new(false);
    private static string _programName = "";

    public static void InterruptibleSleep(int sec)
    {
        var sleptSec = 0;
        while (sleptSec < sec)
        {
            Thread.Sleep(1000);
            ++sleptSec;
            if (EvFlag.Value) return;
        }
        return;
    }

    private static void HandleSignal(object _, ConsoleCancelEventArgs args)
    {
        args.Cancel = true;
        EvFlag.Value = true;
        Console.WriteLine($"SIGINT (Ctrl-C) received, {_programName} will exit gracefully");
    }


    /// <summary>
    /// Register signal handler for SIGINT (Ctrl-C) signal
    /// evFlag will be set to true when SIGINT is received
    /// </summary>
    /// <param name="programName"></param>
    /// <returns>evFlag</returns>
    public static Reference<bool> RegisterSignalHandler(string programName)
    {
        _programName = programName;
        Console.CancelKeyPress += HandleSignal;
        return EvFlag;
    }
}
