using IpcPcQueue.Queue;

var q = new MpscQueue("myQueue", true);
q.Init();
var msg = new byte[512];
Console.WriteLine("Waiting for messages...");
while (true)
{
    var recvSize = q.Dequeue(ref msg);
    if (recvSize > 0)
        Console.WriteLine($"msg: {System.Text.Encoding.UTF8.GetString(msg[..recvSize])}");
    
}

