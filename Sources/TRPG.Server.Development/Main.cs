using TRPG.Common;
using TRPG.Core;
using TRPG.Networking;

namespace TRPG;

static class Program
{
    const int Port = 60000;

    static async Task Main()
    {
        using CancellationTokenSource shutdownCancellation = new CancellationTokenSource();
        Console.CancelKeyPress += (_, eventArgs) =>
        {
            eventArgs.Cancel = true;
            shutdownCancellation.Cancel();
        };

        // TCP 서버
        TcpServer tcpServer = new TcpServer(Port);
        {
            NetResult startAcceptResult = await tcpServer.StartAcceptAsync();
            if (startAcceptResult.IsFailed)
            {
                Console.WriteLine(startAcceptResult.Error.ToString());
                return;
            }

            NetResult startWriteResult = await tcpServer.StartWriteAsync();
            if (startWriteResult.IsFailed)
            {
                Console.WriteLine(startWriteResult.Error.ToString());
                return;
            }
        }

        Console.WriteLine($"server listening. port={Port}");

        while (!shutdownCancellation.IsCancellationRequested)
        {
            Time.Instance.Update();

            // TCP 수신 패킷 처리
            int requestCount = tcpServer.RecvQueue.Count;
            for (int i = 0, count = requestCount; i < count; i++)
            {
                // 수신 패킷 없음?
                if (!tcpServer.RecvQueue.TryDequeue(out var context))
                {
                    break;
                }

                var payloadType = context.GetPayloadType();
                Console.WriteLine($"recv: {payloadType}");

                PacketHandler? handle = PacketHandleHelper.Get(payloadType);
                if (handle is null)
                {
                    Console.WriteLine($"unknown packet: {payloadType}");
                    continue;
                }

                handle.Invoke(ref context);
            }

            // 임시 busy wait 방지
            await Task.Delay(1);
        }

        tcpServer.Close();
    }
}
