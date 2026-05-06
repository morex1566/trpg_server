using Net.Common;
using System;
using System.Net;
using System.Net.Sockets;

namespace Net.Core;

public sealed class Tcp : IDisposable
{
    public enum StateType
    {
        Disconnected,
        Connecting,
        Connected
    }

    private volatile int currentState = (int)StateType.Disconnected;

    private string host = string.Empty;

    private int port = 0;

    private Connection? connection = null;



    public StateType CurrentState => (StateType)currentState;



    public void Init(string host, int port)
    {
        this.host = host;
        this.port = port;
    }

    public async Task<NetResult> AsyncConnect()
    {
        // 이미 connected 임 or connecting 임
        if (Interlocked.CompareExchange(ref currentState, (int)StateType.Connecting, (int)StateType.Disconnected) != (int)StateType.Disconnected)
        {
            return NetResult.Fail(new NetError(NetErrorType.AlreadyConnected, 0, NetErrorType.AlreadyConnected.ToString()));
        }

        CancellationToken cancellationToken = new CancellationTokenSource().Token;

        // DNS 변환 실패함?
        NetResult<IPAddress> resolveResult = await AsyncPostResolve(cancellationToken);
        if (resolveResult.IsFailed)
        {
            Interlocked.Exchange(ref currentState, (int)StateType.Disconnected);
            return NetResult.Fail(resolveResult.Error);
        }

        // 소캣 연결 실패함?
        NetResult<Socket> connectResult = await AsyncPostConnect(resolveResult.Value, cancellationToken);
        if (connectResult.IsFailed)
        {
            Interlocked.Exchange(ref currentState, (int)StateType.Disconnected);
            return NetResult.Fail(connectResult.Error);
        }

        // 연결 성공
        connection = new Connection(connectResult.Value);
        Interlocked.Exchange(ref currentState, (int)StateType.Connected);

        return NetResult.Success();
    }

    public void Close()
    {
        // 연결 완료 상태가 아니면 close 안 함
        if (Interlocked.CompareExchange(ref currentState, (int)StateType.Disconnected, (int)StateType.Connected) != (int)StateType.Connected) return;

        connection?.Dispose();
    }

    public void Dispose()
    {
        Close();
    }

    /// <summary>
    /// DNS 변환
    /// </summary>
    private async Task<NetResult<IPAddress>> AsyncPostResolve(CancellationToken cancellationToken)
    {
        // 이미 connected 임
        if (currentState != (int)StateType.Connecting)
        {
            return NetResult<IPAddress>.Fail(new NetError(NetErrorType.ResolveFailed, 0, NetErrorType.ResolveFailed.ToString()));
        }

        try
        {
            IPAddress[] addresses = await Dns.GetHostAddressesAsync(host, cancellationToken);
            if (addresses.Length == 0)
            {
                return NetResult<IPAddress>.Fail(new NetError(NetErrorType.ResolveFailed, 0, NetErrorType.ResolveFailed.ToString()));
            }

            // DNS 변환 성공
            return NetResult<IPAddress>.Success(addresses[0]);

        }
        catch (Exception exception)
        {
            // DNS 변환 실패
            return NetResult<IPAddress>.Fail(new NetError(NetErrorType.ResolveFailed, 0, exception.Message));
        }
    }

    /// <summary>
    /// host, port로 소캣 연결
    /// </summary>
    private async Task<NetResult<Socket>> AsyncPostConnect(IPAddress address, CancellationToken cancellationToken)
    {
        // connect 들어가기 전에 이미 close() 됨?
        if (currentState != (int)StateType.Connecting)
        {
            return NetResult<Socket>.Fail(new NetError(NetErrorType.AlreadyConnecting, 0, NetErrorType.AlreadyConnecting.ToString()));
        }

        // 빈 소캣
        Socket socket = new Socket(address.AddressFamily, SocketType.Stream, ProtocolType.Tcp)
        {
            NoDelay = true
        };

        try
        {
            await socket.ConnectAsync(new IPEndPoint(address, port), cancellationToken);

            // connect 성공
            return NetResult<Socket>.Success(socket);
        }
        catch (Exception exception)
        {
            // connect 실패
            return NetResult<Socket>.Fail(new NetError(NetErrorType.ConnectFailed, 0, exception.Message));
        }
    }
}
