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
        }

        ShellView shell = new ShellView();
        {
            shell.Submitted += text =>
            {
                shell.Append($"input: {text}");

                // 이후 Tcp.SendAsync 같은 API가 생기면 여기서 호출
                // 지금 Tcp 클래스에는 Send/Receive가 아직 없음
            };

            shell.Run();
        }

        while (tcpClient.CurrentState == Tcp.StateType.Connected)
        {
            timer.Update();
        }

        tcpClient.Close();
    }
}
