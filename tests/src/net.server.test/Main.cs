using Net.Common;
using Net.Core;

const string tcpHost = "127.0.0.1";
const int tcpPort = 60000;

Log logger = Log.GetInstance();
{
    logger.Init();
}

Time timer = Time.GetInstance();
{
    timer.Update();
}

Tcp tcpClient = Tcp.GetInstance();
{
    tcpClient.Init(args.Length > 0 ? args[0] : tcpHost, args.Length > 1 ? int.Parse(args[1]) : tcpPort);
    tcpClient.AsyncConnect();
}

while (tcpClient.GetState() == Tcp.State.Connecting)
{
    timer.Update();
    await Task.Delay(10);
}

Console.WriteLine($"tcp_client state : {tcpClient.GetState()}");
tcpClient.Close();
