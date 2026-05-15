using Terminal.Gui;

namespace TRPG;

public sealed class TerminalApp
{
    private TextField hostField = null!;

    private TextField portField = null!;

    private ComboBox commandCombo = null!;

    private TextField messageField = null!;

    private TextView logView = null!;

    private Label statusLabel = null!;

    private Label guidLabel = null!;


    public event Func<string, int, Task>? ConnectRequested;

    public event Action? DisconnectRequested;

    public event Action<string>? ChatRequested;


    public void Run(Func<bool> tick)
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

            Application.MainLoop.AddTimeout(TimeSpan.FromMilliseconds(50), _ => tick());
            Application.Run();
        }
        finally
        {
            Application.Shutdown();
        }
    }

    public void ClearMessage()
    {
        messageField.Text = "";
    }

    public void AppendLog(string message)
    {
        logView.Text += $"{DateTime.Now:HH:mm:ss} {message}{Environment.NewLine}";
        logView.SetNeedsDisplay();
    }

    public void SetConnectionStatus(bool connected, ulong guid)
    {
        statusLabel.Text = connected ? "Connected" : "Disconnected";
        guidLabel.Text = connected && guid != 0 ? $"GUID: {guid}" : "GUID: -";
        statusLabel.SetNeedsDisplay();
        guidLabel.SetNeedsDisplay();
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
                if (!int.TryParse(portField.Text.ToString(), out int port))
                {
                    AppendLog("invalid port.");
                    break;
                }

                Func<string, int, Task>? connectRequested = ConnectRequested;
                if (connectRequested is not null)
                {
                    await connectRequested(hostField.Text.ToString() ?? "", port);
                }
                break;

            case "disconnect":
                Action? disconnectRequested = DisconnectRequested;
                disconnectRequested?.Invoke();
                break;

            case "chat":
                Action<string>? chatRequested = ChatRequested;
                chatRequested?.Invoke(messageField.Text.ToString() ?? "");
                break;

            default:
                AppendLog($"unknown command: {command}");
                break;
        }
    }
}
