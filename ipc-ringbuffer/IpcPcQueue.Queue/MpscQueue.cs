using System.Runtime.InteropServices;

namespace IpcPcQueue.Queue;

public unsafe class MpscQueue : SpscQueue, IQueue
{
    private readonly int _maxProducerCount;

    /// <summary>
    /// 
    /// </summary>
    /// <param name="queueName">Name of the queue, all producers and the consumer must use the same name</param>
    /// <param name="ownership">Whether this process owns the queue, i.e., it is responsible for the allocation and release the resources used by the queue</param>
    /// <param name="capacity">The lower bound of the max number of messages this queue can hold</param>
    /// <param name="maxMsgSize">The max size in bytes a message can have</param>
    /// <param name="maxProducerCount">The max number of producers that can write to this queue</param>
    public MpscQueue(string queueName, bool ownership = false, int capacity = 1000, int maxMsgSize = 128,
        int maxProducerCount = 8) :
        base(queueName, ownership, capacity, maxMsgSize)
    {
        _maxProducerCount = maxProducerCount;
        QueueSize = capacity * (MaxElementSize + maxProducerCount + 1);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="msgBytes">Enqueue() does not take ownership of msgBytes, feel free to reused it</param>
    /// <returns></returns>
    public new bool Enqueue(byte[] msgBytes)
    {
        var msgLength = msgBytes.Length;
        if (msgLength > MaxMsgSize)
            throw new ArgumentException($"Message length {msgLength} exceeds max element size {MaxMsgSize}");

        var headPtr = (int*)BasePtr;
        var tailPtr = (int*)(BasePtr + sizeof(int));
        var dataOffset = BasePtr + HeaderSize;

        var head = Volatile.Read(ref *headPtr);

        int tail;
        int newTail;

        do
        {
            tail = Volatile.Read(ref *tailPtr);
            var used = GetUsedBytes(head, tail);
            var free = QueueSize - used;

            if (free < MaxElementSize * (_maxProducerCount + 1))
                return false;


            if (tail + MaxElementSize >= QueueSize)
            {
                if (QueueSize - tail >= sizeof(int))
                    // Write a wrap marker (-1) to signal a jump to the start.
                    *(int*)(dataOffset + tail) = FLAG_WRAPPED;

                newTail = MaxElementSize; // Funny, newTail will be MaxElementSize
            }
            else
            {
                newTail = tail + MaxElementSize;
            }
            /*
                reserves space by doing a CAS: compares a value in a memory location with an expected value and,
                if they're equal, replaces it with a new value.
                function CompareExchange(location, newValue, expectedValue)
             */
        } while (Interlocked.CompareExchange(ref *tailPtr, newTail, tail) != tail);

        var msgOffset = tail;


        if (msgOffset + MaxElementSize > QueueSize)
        {
            if (QueueSize - msgOffset >= sizeof(int))
                // Write a wrap marker (-1) to signal a jump to the start.
                *(int*)(dataOffset + msgOffset) = FLAG_WRAPPED;
            msgOffset = 0; // Wrapped, new position starts at the beginning
        }

        // Write payload first.
        //Marshal.Copy(msgBytes, 0, new nint(dataOffset + msgOffset + sizeof(int)), msgLength);
        //Buffer.BlockCopy(msgBytes, 0, new nint(dataOffset), destinationOffset, count);
        fixed (byte* sourcePtr = msgBytes) // Pin the source array
        {
            Buffer.MemoryCopy(sourcePtr, (dataOffset + msgOffset + sizeof(int)), msgLength, msgLength);
        }
        // then write the length field.
        Volatile.Write(ref *(int*)(dataOffset + msgOffset), msgLength);

        return true;
    }
}