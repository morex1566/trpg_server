using Net.Common;
using Net.Core;

static partial class Program
{
    const string host = "127.0.0.1";
    const int port = 60000;

    static async Task Main()
    {
        Log logger = Log.GetInstance();
        {
            logger.Init();
        }

        Time timer = Time.GetInstance();
        {
            timer.Update();
        }

        Tcp tcpClient = new Tcp();
        {
            tcpClient.Init(host, port);

            NetResult connectResult = await tcpClient.AsyncConnect();

            // 연결 실패
            if (connectResult.IsFailed)
            {
                Log.Error(connectResult.Error.ToString());
                return;
            }
            // 연결 성공
            else 
            {
                Log.Temp("Connected.");
            }
        }

        while (tcpClient.CurrentState == Tcp.StateType.Connected)
        {
            timer.Update();
        }

        tcpClient.Close();
    }
}
