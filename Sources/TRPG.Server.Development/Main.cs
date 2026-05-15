using TRPG.Common;
using TRPG.Core;
using TRPG.Networking;

namespace TRPG;

static class Program
{
    const int Port = 60000;

    static async Task Main()
    {
        // TCP 서버
        var tcp = new TcpServer(Port);
        {
            tcp.StartAsyncAccept();
            tcp.StartAsyncWrite();
        }

        while (true)
        {
            Time.Instance.Update();

            // TCP 수신 패킷 처리
            int requestCount = tcp.RecvQueue.Count;
            for (int i = 0, count = requestCount; i < count; i++)
            {
                if (!tcp.RecvQueue.TryDequeue(out var context)) break;

                PacketHandler? handle = PacketHandleHelper.Get(context.GetPayloadType());
                handle?.Invoke(ref context);
            }

            // 임시 busy wait 방지
            await Task.Delay(1);
        }
    }
}
