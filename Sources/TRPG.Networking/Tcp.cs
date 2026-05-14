using System.Collections.Concurrent;
using System.Net;
using System.Net.Sockets;
using TRPG.Common;
using TRPG.Protocol;

namespace TRPG.Networking;

/// <summary>
/// TCP 서버 accept와 connection 목록 관리
/// </summary>
public sealed class Tcp
{
    /// <summary>
    /// tcp 서버 실행 상태
    /// </summary>
    [Flags]
    public enum State
    {
        None = 0,
        AcceptEnabled = 1 << 0,
        WriteEnabled = 1 << 1,
    }


    /// <summary>
    /// 서버 실행 상태
    /// </summary>
    private AtomicFlag<State> currentState;

    /// <summary>
    /// tcp accept / close / write 직렬화용
    /// </summary>
    private readonly Strand strand;

    /// <summary>
    /// Rate Limit용
    /// </summary>
    private readonly TokenBucket acceptTokenBucket;

    /// <summary>
    /// accept용
    /// </summary>
    private TcpListener listener;

    /// <summary>
    /// 연결된 클라이언트 목록
    /// </summary>
    private readonly ConcurrentDictionary<ulong, Connection> connections = new();

    /// <summary>
    /// 검증 완료된 수신 패킷 처리 큐
    /// </summary>
    private readonly ConcurrentQueue<PacketContext> recvQueue = new();

    /// <summary>
    /// write tick-rate용 타이머 취소
    /// </summary>
    private CancellationTokenSource? writeCancellation;

    /// <summary>
    /// accept 루프 취소
    /// </summary>
    private CancellationTokenSource? acceptCancellation;


    public ConcurrentQueue<PacketContext> RecvQueue => recvQueue;


    /// <summary>
    /// tcp 인스턴스 생성
    /// </summary>
    public Tcp(int port)
    {
        strand = new Strand();
        listener = new TcpListener(IPAddress.Any, port);
        acceptTokenBucket = new TokenBucket(SystemConfig.TcpAcceptTokenBucket.Capacity,SystemConfig.TcpAcceptTokenBucket.RefillIntervalMs);
        currentState = new AtomicFlag<State>(State.None);
    }


    /// <summary>
    /// 클라이언트 접속 받기 시작
    /// </summary>
    public void AsyncAccept()
    {
        strand.Post(() =>
        {
            // 이미 AcceptEnabled 상태?
            if (!currentState.TrySet(State.AcceptEnabled)) return Task.CompletedTask;

            // 클라 받기 루프 시작
            listener.Start();
            acceptCancellation = new CancellationTokenSource();
            Task.Run(() => AsyncAcceptLoop(acceptCancellation.Token));

            return Task.CompletedTask;
        });
    }

    /// <summary>
    /// accept 루프
    /// </summary>
    private async Task AsyncAcceptLoop(CancellationToken cancellation)
    {
        while (!cancellation.IsCancellationRequested)
        {
            NetResult<TcpClient> acceptResult = await TryAcceptAsync(cancellation);

            // TryAcceptAsync 실패 (심각)
            if (acceptResult.IsFailed && (acceptResult.Error.Type is NetErrorType.Canceled or NetErrorType.Disposed))
            {
                return;
            }

            // TryAcceptAsync 실패 (주의)
            if (acceptResult.IsFailed)
            {
                continue;
            }

            // AcceptEnabled 상태 아님?
            // 지금 들어온 클라 버림
            // AcceptEnabled 종료
            if (!currentState.Has(State.AcceptEnabled))
            {
                acceptResult.Value.Close();
                return;
            }

            // Rate Limit 걸림?
            // 지금 들어온 클라 버림
            if (!acceptTokenBucket.Consume())
            {
                acceptResult.Value.Close();
                continue;
            }

            // 연결을 받을 수 있을 만큼 메모리 충분?
            // 지금 들어온 클라 버림
            if (SystemConfig.CurrentRamPercentage() > SystemConfig.LimitRamPercentage)
            {
                acceptResult.Value.Close();
                continue;
            }

            // 커넥션 등록
            ulong newConnectionId = ConnectionIdGenerator.Generate();
            ulong newConnectionGuid = GuidGenerator.Generate();
            var newConnection = new Connection(acceptResult.Value, newConnectionGuid, recvQueue, () =>
            {
                connections.TryRemove(newConnectionId, out _);
            });
            newConnection.AsyncRead();
            connections[newConnectionId] = newConnection;
        }
    }

    private async Task<NetResult<TcpClient>> TryAcceptAsync(CancellationToken cancellation)
    {
        try
        {
            TcpClient client = await listener.AcceptTcpClientAsync(cancellation);
            return NetResult<TcpClient>.Success(client);
        }
        catch (OperationCanceledException)
        {
            return NetResult<TcpClient>.Fail(new NetError(NetErrorType.Canceled));
        }
        catch (ObjectDisposedException)
        {
            return NetResult<TcpClient>.Fail(new NetError(NetErrorType.Disposed));
        }
        catch (SocketException)
        {
            return NetResult<TcpClient>.Fail(new NetError(NetErrorType.Socket));
        }
    }

    /// <summary>
    /// write tick-rate마다 모든 Connection에 AsyncWrite 명령
    /// </summary>
    public void AsyncWrite()
    {
        strand.Post(() =>
        {
            // 이미 WriteEnabled 상태?
            if (!currentState.TrySet(State.WriteEnabled)) return Task.CompletedTask;

            // 쓰기 루프 시작
            writeCancellation = new CancellationTokenSource();
            Task.Run(() => AsyncWriteLoop(writeCancellation.Token));

            return Task.CompletedTask;
        });
    }
        
    /// <summary>
    /// Tick-rate마다 모든 connection에 async_write 명령
    /// </summary>
    private async Task AsyncWriteLoop(CancellationToken cancellation)
    {
        var interval = TimeSpan.FromMilliseconds(SystemConfig.Tcp.TickIntervalMs);
        using var timer = new PeriodicTimer(interval);

        while (true)
        {
            NetResult<bool> waitResult = await TryWaitForNextWriteTickAsync(timer, cancellation);

            // TryWaitForNextWriteTickAsync 실패 (ex. 작업 취소)
            if (waitResult.IsFailed)
            {
                return;
            }

            // timer 종료. Dispose 됨
            if (!waitResult.Value)
            {
                return;
            }

            // WriteEnabled 상태 아님?
            if (!currentState.Has(State.WriteEnabled))
            {
                return;
            }

            // 모든 커넥션에 Write 명령
            foreach (var connection in connections)
            {
                connection.Value.AsyncWrite();
            }
        }
    }

    private static async Task<NetResult<bool>> TryWaitForNextWriteTickAsync(PeriodicTimer timer, CancellationToken cancellation)
    {
        try
        {
            bool hasNextTick = await timer.WaitForNextTickAsync(cancellation);
            return NetResult<bool>.Success(hasNextTick);
        }
        catch (OperationCanceledException)
        {
            return NetResult<bool>.Fail(new NetError(NetErrorType.Canceled));
        }
        catch (ObjectDisposedException)
        {
            return NetResult<bool>.Fail(new NetError(NetErrorType.Disposed));
        }
    }

    /// <summary>
    /// tcp 서버 종료
    /// </summary>
    public void Close()
    {
        strand.Post(() =>
        {
            // Write() 취소
            currentState.TryUnset(State.WriteEnabled);
            writeCancellation?.Cancel();

            // Accept() 취소
            currentState.TryUnset(State.AcceptEnabled);
            acceptCancellation?.Cancel();
            try
            {
                listener.Stop();
            }
            catch 
            {
            }

            // 연결 종료
            foreach (var connection in connections.Values.ToArray())
            {
                connection.Close();
            }
            connections.Clear();

            return Task.CompletedTask;
        });
    }
}
