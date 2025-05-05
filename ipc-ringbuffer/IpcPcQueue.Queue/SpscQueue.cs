using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;

namespace IpcPcQueue.Queue;

public unsafe class SpscQueue : IQueue, IDisposable
{
    protected const int FLAG_WRAPPED = -1;
    private const int FLAG_MSG_UNCOMMITTED = -2;
    private readonly string _mappedFileName = "Local\\IpcPcQueue_";
    protected const int HeaderSize = sizeof(int) * 2;
    protected int QueueSize;
    protected readonly int MaxMsgSize;
    protected readonly int MaxElementSize;
    protected byte* BasePtr = null;
    private readonly bool _ownership;
    private MemoryMappedFile _mmf;
    private MemoryMappedViewAccessor _accessor;

    public void Init()
    {
        var totalSize = HeaderSize + QueueSize;
        totalSize += sizeof(int) - totalSize % sizeof(int); // totalSize must be aligned to 4 bytes
        _mmf = MemoryMappedFile.CreateOrOpen(_mappedFileName, totalSize, MemoryMappedFileAccess.ReadWrite);
        _accessor = _mmf.CreateViewAccessor(0, totalSize, MemoryMappedFileAccess.ReadWrite);
        _accessor.SafeMemoryMappedViewHandle.AcquirePointer(ref BasePtr);

        if (!_ownership) return;
        var intPtr = (int*)BasePtr;

        var numberOfIntegers = totalSize / sizeof(int);
        for (var i = 0; i < numberOfIntegers; i++) intPtr[i] = FLAG_MSG_UNCOMMITTED;

        _accessor.Write(0, 0); // head
        _accessor.Write(4, 0); // tail
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="queueName">Name of the queue, producer and consumer must use the same name</param>
    /// <param name="ownership">Whether this process owns the queue, i.e., it is responsible for the allocation and release the resources used by the queue</param>
    /// <param name="capacity">The lower bound of the max number of messages this queue can hold</param>
    /// <param name="maxMsgSize">The max size in bytes a message can have</param>
    public SpscQueue(string queueName, bool ownership = false, int capacity = 1000, int maxMsgSize = 128)
    {
        _ownership = ownership;
        _mappedFileName += queueName;
        MaxMsgSize = maxMsgSize;
        MaxElementSize = MaxMsgSize + sizeof(int); // 4 bytes for the length field plus the payload.
        MaxElementSize += sizeof(int) - MaxElementSize % sizeof(int); // maxElementSize must be aligned to 4 bytes
        QueueSize = capacity * (MaxElementSize + 1);
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="msgBytes">Enqueue() does not take ownership of msgBytes, feel free to reused it</param>
    /// <returns></returns>
    public bool Enqueue(byte[] msgBytes)
    {
        var msgLength = msgBytes.Length;
        if (msgLength > MaxMsgSize)
            throw new ArgumentException($"Message length {msgLength} exceeds max element size {MaxMsgSize}");

        var headPtr = (int*)BasePtr;
        var tailPtr = (int*)(BasePtr + sizeof(int));
        var dataOffset = BasePtr + HeaderSize;

        var head = Volatile.Read(ref *headPtr);
        // C#'s equivalent of
        // auto tail = tailPtr.load(std::memory_order_relaxed);
        var tail = Volatile.Read(ref *tailPtr);

        var used = GetUsedBytes(head, tail);
        var free = QueueSize - used;

        if (free < MaxElementSize * 2)
            return false;

        var msgOffset = tail;
        // If the record would not fit contiguously, write a wrap marker and wrap.
        if (msgOffset + MaxElementSize > QueueSize)
        {
            if (QueueSize - msgOffset >= sizeof(int))
                // Write a wrap marker (-1) to signal a jump to the start.
                *(int*)(dataOffset + msgOffset) = FLAG_WRAPPED;

            msgOffset = 0;
        }

        // Write payload first.
        Marshal.Copy(msgBytes, 0, new nint(dataOffset + msgOffset + sizeof(int)), msgLength);
        // then write the length field.
        Volatile.Write(ref *(int*)(dataOffset + msgOffset), msgLength);

        // Each time we move the tail, we have to move by _maxElementSize, instead of (sizeof(int) + msgLength)
        // why not using a variable-length slot design? The tricky part is that we are writing FLAG_MSG_UNCOMMITTED
        // to denote that a slot is "allocated, but not committed". This is a MPSC-specific design.
        // Using this design, each time we move the tail, (queueBase + head) must be aligned,
        var newTail = msgOffset + MaxElementSize;
        if (newTail >= QueueSize) newTail = 0;

        // C#'s equivalent of
        // tailPtr.store(newTail, std::memory_order_release);
        Volatile.Write(ref *tailPtr, newTail);


        return true;
    }

    /// <summary>
    /// 
    /// </summary>
    /// <param name="buffer">Received message will be written to this buffer. It is caller's responsibility to make sure the buffer is large enough</param>
    /// <returns>size of the message or -1 if no new message is available</returns>
    public int Dequeue(ref byte[] buffer)
    {
        var headPtr = (int*)BasePtr;
        var tailPtr = (int*)(BasePtr + sizeof(int));
        var queueBase = BasePtr + HeaderSize;

        var head = Volatile.Read(ref *headPtr);
        var tail = Volatile.Read(ref *tailPtr);
        //Console.WriteLine($"Dequeue(), head: {head}, tail: {tail}");
        if (head == tail)
            return -1; // No message available.

        var msgLength = Volatile.Read(ref *(int*)(queueBase + head));
        if (msgLength == FLAG_WRAPPED)
        {
            head = 0;
            Volatile.Write(ref *headPtr, head);
            msgLength = *(int*)(queueBase + head);
        }

        if (msgLength == FLAG_MSG_UNCOMMITTED) // MPSC-specific, slot allocated, but not committed yet
            return -1;

        Volatile.Write(ref *(int*)(queueBase + head), FLAG_MSG_UNCOMMITTED); // MPSC-specific, mark slot as uncommitted
        //Marshal.Copy(new nint(queueBase + head + 4), buffer, 0, msgLength);
        fixed (byte* destPtr = buffer)
        {
            Buffer.MemoryCopy(queueBase + head + sizeof(int), destPtr, msgLength, msgLength);
        }

        // Move head pointer to next element.
        var newHead = head + MaxElementSize;

        Volatile.Write(ref *headPtr, newHead);
        return msgLength;
    }

    public void Dispose()
    {
        _accessor.SafeMemoryMappedViewHandle.ReleasePointer();
        _accessor?.Dispose();
        if (_ownership)
        {
            _mmf?.Dispose();
            Console.WriteLine($"This process owns the queue at [{_mappedFileName}], so it will release the queue now.");
        }
        else
        {
            Console.WriteLine(
                $"This process does NOT own the queue at [{_mappedFileName}], so it will NOT release the queue.");
        }
    }

    public int GetUsedBytes(int head = -1, int tail = -1)
    {
        if (head == -1)
        {
            var headPtr = (int*)BasePtr;
            head = Volatile.Read(ref *headPtr);
        }

        if (tail == -1)
        {
            var tailPtr = (int*)(BasePtr + sizeof(int));
            tail = Volatile.Read(ref *tailPtr);
        }

        if (tail >= head)
            return tail - head;
        return QueueSize - (head - tail);
    }

    public int Head()
    {
        var headPtr = (int*)BasePtr;
        return Volatile.Read(ref *headPtr);
    }

    public int Tail()
    {
        var tailPtr = (int*)(BasePtr + sizeof(int));
        return Volatile.Read(ref *tailPtr);
    }
}