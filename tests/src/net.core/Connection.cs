using Net.Common;
using System.Net.Sockets;

namespace Net.Core;

public sealed class Connection : IDisposable
{
    private readonly Socket socket;
    private readonly ulong connectionId;
    private int isClosed;

    public Connection(Socket socket, ulong connectionId)
    {
        this.socket = socket;
        this.connectionId = connectionId;
        Log.GetInstance().Info($"create {Log.Demangle(typeof(Connection))} instance.");
    }

    public void Close()
    {
        if (Interlocked.Exchange(ref isClosed, 1) == 1) return;

        try
        {
            if (socket.Connected)
            {
                socket.Shutdown(SocketShutdown.Both);
            }
        }
        catch (SocketException exception)
        {
            Log.GetInstance().Warn($"connection shutdown failed: {exception.Message}");
        }
        catch (ObjectDisposedException)
        {
        }

        socket.Dispose();
    }

    public void Dispose()
    {
        Close();
        Log.GetInstance().Info($"destroy {Log.Demangle(typeof(Connection))} instance.");
    }
}
