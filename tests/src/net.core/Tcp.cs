using System.Net;
using Net.Common;
using System.Net.Sockets;

namespace Net.Core;

public sealed class Tcp : GlobalSingleton<Tcp>, IDisposable
{
    public enum State
    {
        Disconnected,
        Connecting,
        Connected
    }

    private int currentState = (int)State.Disconnected;

    private string host = string.Empty;

    private int port;

    private CancellationTokenSource? connectCancellationSource;

    private Connection? connection;



    public Tcp()
    {
        Log.GetInstance().Info($"create {Log.Demangle(typeof(Tcp))} instance.");
    }



    public void Init(string host, int port)
    {
        this.host = host;
        this.port = port;
    }

    public void AsyncConnect()
    {
        if (Interlocked.CompareExchange(ref currentState, (int)State.Connecting, (int)State.Disconnected) != (int)State.Disconnected) return;

        connectCancellationSource = new CancellationTokenSource();
        _ = PostResolve(connectCancellationSource.Token);
    }

    public void Close()
    {
        if (Interlocked.Exchange(ref currentState, (int)State.Disconnected) == (int)State.Disconnected) return;

        connectCancellationSource?.Cancel();
        connectCancellationSource?.Dispose();
        connectCancellationSource = null;

        connection?.Close();
        connection = null;
    }

    public State GetState()
    {
        return (State)currentState;
    }

    public void Dispose()
    {
        Close();
    }

    private async Task PostResolve(CancellationToken cancellationToken)
    {
        if (currentState != (int)State.Connecting) return;

        try
        {
            IPAddress[] addresses = await Dns.GetHostAddressesAsync(host, cancellationToken);
            await PostConnect(addresses, cancellationToken);
        }
        catch (Exception exception) when (exception is not OperationCanceledException)
        {
            Log.GetInstance().Warn($"resolve error : {exception.Message}.");
            Volatile.Write(ref currentState, (int)State.Disconnected);
        }
    }

    private async Task PostConnect(IPAddress[] addresses, CancellationToken cancellationToken)
    {
        if (GetState() != State.Connecting) return;

        Exception? lastException = null;

        foreach (IPAddress address in addresses)
        {
            Socket socket = new(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

            try
            {
                await socket.ConnectAsync(address, port, cancellationToken);

                connection = new Connection(socket, 0);
                Volatile.Write(ref currentState, (int)State.Connected);
                return;
            }
            catch (Exception exception) when (exception is not OperationCanceledException)
            {
                lastException = exception;
                socket.Dispose();
            }
        }

        Log.GetInstance().Warn($"socket error : {lastException?.Message ?? "connect failed"}.");
        Volatile.Write(ref currentState, (int)State.Disconnected);
    }
}
