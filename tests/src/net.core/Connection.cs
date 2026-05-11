using System.Net.Sockets;

namespace Net.Core;

public sealed class Connection : IDisposable
{
    public enum StateType
    {
        None,
        Disconnected,
        Connected
    }

    private volatile int currentState = (int)StateType.None;
    private Socket socket;
    private ulong connectionId;

    public Connection(Socket socket)
    {
        this.socket = socket;
        currentState = (int)StateType.Connected;
    }

    public void Close()
    {
        if (Interlocked.CompareExchange(ref currentState, (int)StateType.Disconnected, (int)StateType.Connected) != (int)StateType.Connected) return;

        socket.Dispose();
    }

    public void Dispose()
    {
        Close();
    }
}
