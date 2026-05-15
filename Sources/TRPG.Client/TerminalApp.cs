using System.Collections.Concurrent;
using System.Net.Sockets;
using Terminal.Gui;
using TRPG.Networking;
using TRPG.Protocol;

namespace TRPG;

public sealed class TerminalApp
{
    private readonly ConcurrentQueue<PacketContext> recvQueue = new();

    private TcpClient? socket;
    private Connection? connection;
    private bool connectionClosed;

    private TextField hostField = null!;
    private TextField portField = null!;
    private ComboBox commandCombo = null!;
    private TextField messageField = null!;
    private TextView logView = null!;
    private Label statusLabel = null!;
    private Label guidLabel = null!;

    private bool IsConnected => connection is not null && !connectionClosed;

    public void Run()
    {
        Application.Init();

        try
        {
            var top = Application.Top;
            var window = new Window("TRPG Client")
            {
                X = 0,
                Y = 1,
                Width = Dim.Fill(),
                Height = Dim.Fill()
            };

            top.Add(window);

            BuildLayout(window);
            AppendLog("client ui ready.");

            // 주기적으로 수신 큐를 비우고 로그를 업데이트
            Application.MainLoop.AddTimeout(TimeSpan.FromMilliseconds(50), DrainReceivedPackets);
            Application.Run();
        }
        finally
        {
            Application.Shutdown();
        }
    }

    private void BuildLayout(Window window)
    {
        var hostLabel = new Label("Host")
        {
            X = 1,
            Y = 1,
            Width = 6
        };

        hostField = new TextField("127.0.0.1")
        {
            X = Pos.Right(hostLabel) + 1,
            Y = 1,
            Width = 18
        };

        var portLabel = new Label("Port")
        {
            X = Pos.Right(hostField) + 2,
            Y = 1,
            Width = 6
        };

        portField = new TextField("60000")
        {
            X = Pos.Right(portLabel) + 1,
            Y = 1,
            Width = 8
        };

        commandCombo = new ComboBox()
        {
            X = 1,
            Y = 3,
            Width = 16,
            Height = 4
        };
        commandCombo.SetSource(new[] { "connect", "disconnect", "chat" });
        commandCombo.SelectedItem = 0;

        messageField = new TextField("")
        {
            X = Pos.Right(commandCombo) + 1,
            Y = 3,
            Width = Dim.Fill(10)
        };

        var runButton = new Button("Run")
        {
            X = Pos.Right(messageField) + 1,
            Y = 3
        };
        runButton.Clicked += async () => await ExecuteSelectedCommandAsync();

        statusLabel = new Label("Disconnected")
        {
            X = 1,
            Y = 5,
            Width = 24
        };

        guidLabel = new Label("GUID: -")
        {
            X = Pos.Right(statusLabel) + 2,
            Y = 5,
            Width = 40
        };

        logView = new TextView()
        {
            X = 1,
            Y = 7,
            Width = Dim.Fill(1),
            Height = Dim.Fill(1),
            ReadOnly = true
        };

        window.Add(
            hostLabel,
            hostField,
            portLabel,
            portField,
            commandCombo,
            messageField,
            runButton,
            statusLabel,
            guidLabel,
            logView);
    }

    private async Task ExecuteSelectedCommandAsync()
    {
        string command = commandCombo.Text.ToString() ?? "";
        
        switch (command)
        {
            case "connect":
                await ConnectAsync();
                break;

            case "disconnect":
                Disconnect();
                break;

            case "chat":
                Chat();
                break;

            default:
                AppendLog($"unknown command: {command}");
                break;
        }

        UpdateStatus();
    }

    private async Task ConnectAsync()
    {
        if (IsConnected)
        {
            AppendLog("already connected.");
            return;
        }

        if (!int.TryParse(portField.Text.ToString(), out int port))
        {
            AppendLog("invalid port.");
            return;
        }

        var client = new TcpClient();
        try
        {
            await client.ConnectAsync(hostField.Text.ToString() ?? "127.0.0.1", port);
        }
        catch (Exception ex)
        {
            client.Close();
            AppendLog($"connect failed: {ex.Message}");
            return;
        }

        socket = client;
        connectionClosed = false;
        connection = new Connection(client, 0, recvQueue, OnConnectionClosed);
        connection.StartAsyncRead();
        SendConnectRequest();

        AppendLog("connect requested.");
    }

    private void Disconnect()
    {
        if (!IsConnected)
        {
            AppendLog("already disconnected.");
            return;
        }

        // 사용자의 요청: 별도의 DisconnectRequest 패킷 송신 없이 로컬 소켓만 닫음.
        // 서버의 read 실패 처리를 통해 자동으로 커넥션이 끊어짐.
        CloseLocal();
        AppendLog("disconnected.");
    }

    private void Chat()
    {
        if (!IsConnected)
        {
            AppendLog("not connected.");
            return;
        }

        string message = messageField.Text.ToString() ?? "";
        if (string.IsNullOrWhiteSpace(message))
        {
            AppendLog("empty message.");
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

        SendPacket(packet);
        AppendLog($"me: {message}");
        messageField.Text = "";
    }

    private void SendConnectRequest()
    {
        PacketContext packet = PacketContext.Create(PayloadType.ConnectRequest, builder =>
        {
            ConnectRequest.StartConnectRequest(builder);
            return ConnectRequest.EndConnectRequest(builder).Value;
        });

        SendPacket(packet);
    }

    private void SendPacket(PacketContext packet)
    {
        if (!IsConnected)
        {
            AppendLog("not connected.");
            return;
        }

        connection!.Send(packet.Buffer);
        connection.AsyncWrite();
    }

    private bool DrainReceivedPackets(MainLoop caller)
    {
        if (connectionClosed && connection is not null)
        {
            connection = null;
            socket = null;
            AppendLog("connection closed.");
        }

        while (recvQueue.TryDequeue(out PacketContext context))
        {
            switch (context.GetPayloadType())
            {
                case PayloadType.ConnectResponse:
                {
                    ConnectResponse response = context.PayloadAs<ConnectResponse>()!.Value;
                    connection?.SetGuid(response.Guid);
                    AppendLog($"connected. guid={response.Guid}");
                    break;
                }

                case PayloadType.ChatMessageResponse:
                {
                    ChatMessageResponse response = context.PayloadAs<ChatMessageResponse>()!.Value;
                    AppendLog($"server: {response.Message}");
                    break;
                }
            }
        }

        UpdateStatus();
        return true;
    }

    private void CloseLocal()
    {
        connection?.Close();

        try
        {
            socket?.Close();
        }
        catch
        {
        }

        connection = null;
        socket = null;
        connectionClosed = true;
    }

    private void OnConnectionClosed()
    {
        connectionClosed = true;
    }

    private void UpdateStatus()
    {
        statusLabel.Text = IsConnected ? "Connected" : "Disconnected";
        guidLabel.Text = IsConnected ? $"GUID: {connection?.Guid.ToString() ?? "-"}" : "GUID: -";
    }

    private void AppendLog(string message)
    {
        logView.Text += $"{DateTime.Now:HH:mm:ss} {message}{Environment.NewLine}";
    }
}
