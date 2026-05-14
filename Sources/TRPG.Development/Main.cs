using TRPG.Common;
using TRPG.Core;
using TRPG.Networking;
using TRPG.Protocol;

namespace TRPG;

static class Program
{
    const int TcpPort = 60000;

    static async Task Main()
    {
        // TCP 서버
        var tcp = new Tcp(TcpPort);
        {
            tcp.AsyncAccept();
            tcp.AsyncWrite();
        }

        while (true)
        {
            Time.Instance.Update();

            int requestCount = tcp.RecvQueue.Count;
            for (int i = 0, count = requestCount; i < count; i++)
            {
                if (!tcp.RecvQueue.TryDequeue(out var context))
                {
                    break;
                }

                PacketHandler? handle = PacketHandleHelper.Get(context.GetPayloadType());
                handle?.Invoke(in context);
            }

            // 임시 busy wait 방지
            await Task.Delay(1);
        }
    }
}