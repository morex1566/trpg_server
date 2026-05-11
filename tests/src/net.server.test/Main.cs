using Net.Core;

static partial class Program
{
    private const string defaultHost = "127.0.0.1";
    private const int defaultPort = 60000;

    private static readonly string[] commands =
    {
        "/connect",
        "/disconnect",
        "/status",
        "/help",
        "/clear",
        "/quit"
    };

    static void Main()
    {
        Tcp tcpClient = new Tcp();
        Shell shell = new Shell();

        shell.SetCommands(commands);

        shell.Submitted = async text =>
        {
            await ExecuteAsync(shell, tcpClient, text);
        };

        shell.Run();
        tcpClient.Close();
    }

    private static async Task ExecuteAsync(Shell shell, Tcp tcpClient, string text)
    {
        string[] tokens = text.Split(' ', StringSplitOptions.RemoveEmptyEntries);
        if (tokens.Length == 0) return;

        switch (tokens[0])
        {
            case "/connect":
                await ExecuteConnectAsync(shell, tcpClient, tokens);
                break;

            case "/disconnect":
                tcpClient.Close();
                shell.Append($"state: {tcpClient.CurrentState}");
                break;

            case "/status":
                shell.Append($"state: {tcpClient.CurrentState}");
                break;

            case "/help":
                ExecuteHelp(shell);
                break;

            case "/clear":
                shell.Clear();
                break;

            case "/quit":
                tcpClient.Close();
                shell.Stop();
                break;

            default:
                shell.Append($"unknown command: {tokens[0]}");
                break;
        }
    }

    private static async Task ExecuteConnectAsync(Shell shell, Tcp tcpClient, string[] tokens)
    {
        string host = tokens.Length >= 2 ? tokens[1] : defaultHost;
        int port = defaultPort;

        if (tokens.Length >= 3 && !int.TryParse(tokens[2], out port))
        {
            shell.Append("invalid port.");
            return;
        }

        if (tcpClient.CurrentState == Tcp.StateType.Connected)
        {
            shell.Append("already connected.");
            return;
        }

        tcpClient.Init(host, port);
        shell.Append($"connecting to {host}:{port}...");

        NetResult result = await tcpClient.AsyncConnect();
        if (result.IsFailed)
        {
            shell.Append(result.Error.ToString());
            return;
        }

        shell.Append($"state: {tcpClient.CurrentState}");
    }

    private static void ExecuteHelp(Shell shell)
    {
        shell.Append("/connect [host] [port] - connect to server");
        shell.Append("/disconnect - disconnect from server");
        shell.Append("/status - show connection state");
        shell.Append("/clear - clear terminal messages");
        shell.Append("/quit - quit terminal");
    }
}
