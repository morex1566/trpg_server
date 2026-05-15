using TRPG.Core;
using TRPG.Networking;
using TRPG.Protocol;

namespace TRPG;

static partial class Program
{
    private static TerminalApp app = new TerminalApp();

    private static TcpClient? tcpClient;


    public static void Main()
    {
        app.ConnectRequested += OnConnectAsync;
        app.DisconnectRequested += OnDisconnect;
        app.ChatRequested += OnChat;
        app.Run(Tick);
    }

    private static bool Tick()
    {
        if (tcpClient is null) return true;

        // TCP 수신 패킷 처리
        int requestCount = tcpClient.RecvQueue.Count;
        for (int i = 0, count = requestCount; i < count; i++)
        {
            // 수신 패킷 없음?
            if (!tcpClient.RecvQueue.TryDequeue(out var context))
            {
                break;
            }

            HandlePacket(ref context);
        }

        return true;
    }

    private static bool SendPacket(PacketContext packet)
    {
        if (tcpClient?.IsConnected != true)
        {
            app.AppendLog("not connected.");
            return false;
        }

        NetResult sendResult = tcpClient.Send(packet.Buffer);
        if (sendResult.IsFailed)
        {
            app.AppendLog(sendResult.Error.ToString());
            return false;
        }

        return true;
    }

    private static void HandlePacket(ref PacketContext context)
    {
        var payloadType = context.GetPayloadType();
        app.AppendLog($"recv: {payloadType}");

        switch (payloadType)
        {
            case PayloadType.ConnectResponse:
                ReceiveConnectResponse(ref context);
                break;

            case PayloadType.ChatMessageResponse:
                ReceiveChatMessageResponse(ref context);
                break;

            default:
                app.AppendLog($"unknown packet: {payloadType}");
                break;
        }
    }

    private static void ReceiveConnectResponse(ref PacketContext context)
    {
        ConnectResponse response = context.PayloadAs<ConnectResponse>()!.Value;

        tcpClient?.SetGuid(response.Guid);
        app.SetConnectionStatus(true, response.Guid);
        app.AppendLog($"server guid: {response.Guid}");
    }

    private static void ReceiveChatMessageResponse(ref PacketContext context)
    {
        ChatMessageResponse response = context.PayloadAs<ChatMessageResponse>()!.Value;

        app.AppendLog($"server: {response.Message}");
    }
}

static partial class Program
{
    private static async Task OnConnectAsync(string host, int port)
    {
        if (tcpClient?.IsConnected == true)
        {
            app.AppendLog("already connected.");
            return;
        }

        TcpClient client = new TcpClient(host, port);
        tcpClient = client;

        NetResult startConnectResult = await client.StartConnectAsync();
        if (!ReferenceEquals(tcpClient, client))
        {
            client.Close();
            return;
        }

        if (startConnectResult.IsFailed)
        {
            app.AppendLog(startConnectResult.Error.ToString());
            tcpClient = null;
            return;
        }

        NetResult startWriteResult = await client.StartWriteAsync();
        if (!ReferenceEquals(tcpClient, client))
        {
            client.Close();
            return;
        }

        if (startWriteResult.IsFailed)
        {
            app.AppendLog(startWriteResult.Error.ToString());
            client.Close();
            tcpClient = null;
            return;
        }

        PacketContext packet = PacketContext.Create(PayloadType.ConnectRequest, builder =>
        {
            ConnectRequest.StartConnectRequest(builder);
            return ConnectRequest.EndConnectRequest(builder).Value;
        });

        if (SendPacket(packet))
        {
            app.SetConnectionStatus(true, client.Guid);
            app.AppendLog("connected.");
        }
    }

    private static void OnDisconnect()
    {
        tcpClient?.Close();
        tcpClient = null;

        app.SetConnectionStatus(false, 0);
        app.AppendLog("disconnected.");
    }

    private static void OnChat(string message)
    {
        if (tcpClient?.IsConnected != true)
        {
            app.AppendLog("not connected.");
            return;
        }

        if (string.IsNullOrWhiteSpace(message))
        {
            app.AppendLog("empty message.");
            return;
        }

        PacketContext packet = PacketContext.Create(PayloadType.ChatMessageRequest, builder =>
        {
            var messageOffset = builder.CreateString(message);
            return ChatMessageRequest.CreateChatMessageRequest(
                builder,
                ChatType.ToServer,
                0,
                messageOffset).Value;
        });

        if (SendPacket(packet))
        {
            app.AppendLog($"me: {message}");
            app.ClearMessage();
        }
    }
}
