using System.Text;
using IpcPcQueue.Queue;

var q = new MpscQueue("myQueue");
q.Init();
while (true)
{
    var msgStr = GenerateRandomString();
    if (q.Enqueue(Encoding.UTF8.GetBytes(msgStr)))
        Console.WriteLine($"msg Enqueue()ed: {msgStr}");
    else
        Thread.Sleep(1000);
} 
        
static string GenerateRandomString()
{
    Random random = new Random();
    int length = random.Next(1, 108);
    const string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    StringBuilder sb = new StringBuilder();
    sb.Append(DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
    sb.Append("_");
    for (int i = 0; i < length; i++)
    {
        sb.Append(chars[random.Next(chars.Length)]);
    }

    return sb.ToString();
}
