using System;
using System.Buffers.Binary;
using System.Collections.Concurrent;
using System.IO;
using System.Net.Sockets;
using System.Threading.Tasks;
using Google.FlatBuffers;
using Serilog;
using TRPG.Common;
using TRPG.Protocol;

namespace TRPG.Networking;

/// <summary>
/// 클라이언트 TCP 연결 1개의 송수신 상태 관리
/// </summary>
public sealed class Connection
{
    /// <summary>
    /// connection 연결 상태
    /// </summary>
    [Flags]
    public enum State
    {
        None = 0,

        // TCP에 등록된 상태
        Connected = 1 << 0,

        // AsyncRead() 켜짐
        ReadEnabled = 1 << 1,

        // Write중
        Writing = 1 << 2,
    }


    /// <summary>
    /// connection handler 실행 strand
    /// </summary>
    private readonly Strand strand;

    /// <summary>
    /// 클라이언트 TCP socket
    /// </summary>
    private readonly TcpClient socket;

    /// <summary>
    /// 네트워크 스트림
    /// </summary>
    private readonly NetworkStream stream;

    /// <summary>
    /// 검증 완료된 수신 패킷 처리 큐
    /// </summary>
    private readonly ConcurrentQueue<PacketContext> recvQueue;

    /// <summary>
    /// strand 위에서만 읽고 쓴다
    /// </summary>
    private readonly ConcurrentQueue<byte[]> sendQueue;

    /// <summary>
    /// 클라에게 내려주는 서버 측 고유 GUID
    /// </summary>
    private readonly ulong connectionGuid;

    /// <summary>
    /// AsyncWrite() 중도 취소용
    /// </summary>
    private CancellationTokenSource? writeCancellation;

    /// <summary>
    /// AsyncRead() 중도 취소용
    /// </summary>
    private CancellationTokenSource? readCancellation;

    /// <summary>
    /// 현재 connection 상태
    /// </summary>
    private AtomicFlag<State> currentState;


    public event Action OnClosed;


    /// <summary>
    /// connection 인스턴스 생성
    /// </summary>
    public Connection(TcpClient clientSocket, ulong connectionId, ConcurrentQueue<PacketContext> queue, Action onClosed)
    {
        strand = new Strand();
        socket = clientSocket;
        stream = clientSocket.GetStream();
        connectionGuid = connectionId;
        recvQueue = queue;
        sendQueue = new ConcurrentQueue<byte[]>();
        currentState = new AtomicFlag<State>(State.Connected);
        OnClosed = onClosed;
    }


    public ulong Guid => connectionGuid;


    /// <summary>
    /// 송신 패킷 enqueue 요청
    /// </summary>
    public void Send(byte[] buffer)
    {
        strand.Post(() =>
        {
            if (!currentState.Has(State.Connected))
            {
                return Task.CompletedTask;
            }

            // 송신 큐에 넣을 수 있음?
            if (!TryValidateSend(buffer))
            {
                Close();
                return Task.CompletedTask;
            }

            sendQueue.Enqueue(buffer);

            return Task.CompletedTask;
        });
    }

    /// <summary>
    /// 송신 queue flush 시작
    /// </summary>
    public void AsyncWrite()
    {
        strand.Post(() =>
        {
            if (!currentState.Has(State.Connected))
            {
                return Task.CompletedTask;
            }

            // 이미 쓰는 중?
            if (!currentState.TrySet(State.Writing))
            {
                return Task.CompletedTask;
            }

            writeCancellation = new CancellationTokenSource();
            Task.Run(() => AsyncWriteBatch(writeCancellation.Token));

            return Task.CompletedTask;
        });
    }

    /// <summary>
    /// send_queue 전부 batch로 묶어서 write
    /// </summary>
    private async Task AsyncWriteBatch(CancellationToken cancellation)
    {
        int batchCount = sendQueue.Count;
        for (int i = 0; i < batchCount; i++)
        {
            // 큐가 비어있음?
            if (!sendQueue.TryDequeue(out byte[]? buffer))
            {
                currentState.TryUnset(State.Writing);
                return;
            }

            // Write 실패 (연결 끊김 등)
            NetResult writeResult = await TryWriteAsync(buffer, cancellation);
            if (writeResult.IsFailed)
            {
                currentState.TryUnset(State.Writing);
                Close();
                return;
            }
        }

        currentState.TryUnset(State.Writing);
    }

    private async Task<NetResult> TryWriteAsync(byte[] buffer, CancellationToken cancellation)
    {
        try
        {
            await stream.WriteAsync(buffer, 0, buffer.Length, cancellation);
            return NetResult.Success();
        }
        catch (IOException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Socket, ex.Message));
        }
        catch (ObjectDisposedException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Disposed, ex.Message));
        }
        catch (SocketException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Socket, ex.Message));
        }
    }

    /// <summary>
    /// 수신 read 시작
    /// </summary>
    public void AsyncRead()
    {
        strand.Post(() =>
        {
            if (!currentState.Has(State.Connected))
            {
                return Task.CompletedTask;
            }

            // 이미 ReadEnabled 중?
            if (!currentState.TrySet(State.ReadEnabled))
            {
                return Task.CompletedTask;
            }

            readCancellation = new CancellationTokenSource();
            Task.Run(() => AsyncReadLoop(readCancellation.Token));

            return Task.CompletedTask;
        });
    }

    /// <summary>
    /// header -> payload 연속 읽기 루프
    /// </summary>
    private async Task AsyncReadLoop(CancellationToken cancellation)
    {
        while (!cancellation.IsCancellationRequested)
        {
            // 이미 연결 종료됨?
            if (!currentState.Has(State.Connected))
            {
                return;
            }

            // size prefix header 읽기 실패?
            NetResult<byte[]> headerResult = await AsyncReadHeader(cancellation);
            if (headerResult.IsFailed)
            {
                currentState.TryUnset(State.ReadEnabled);
                Close();
                return;
            }

            byte[] header = headerResult.Value;

            // payload size가 비정상?
            int payloadSize = BinaryPrimitives.ReadInt32LittleEndian(header);
            int packetSize = sizeof(int) + payloadSize;
            if (payloadSize <= 0 || packetSize > SystemConfig.Connection.BufferSize)
            {
                currentState.TryUnset(State.ReadEnabled);
                Close();
                return;
            }

            // 임시 context 생성
            var context = new PacketContext(this, packetSize);
            Buffer.BlockCopy(header, 0, context.Buffer, 0, header.Length);

            // payload 전체 읽기 실패?
            NetResult payloadResult = await AsyncReadPayload(context.Buffer, payloadSize, cancellation);
            if (payloadResult.IsFailed)
            {
                currentState.TryUnset(State.ReadEnabled);
                Close();
                return;
            }

            // flatbuffers verifier 실패?
            if (!TryValidateReceive(ref context))
            {
                currentState.TryUnset(State.ReadEnabled);
                Close();
                return;
            }

            recvQueue.Enqueue(context);
        }

        currentState.TryUnset(State.ReadEnabled);
    }

    /// <summary>
    /// size-prefixed flatbuffer header 4바이트 읽기
    /// </summary>
    private async Task<NetResult<byte[]>> AsyncReadHeader(CancellationToken cancellation)
    {
        byte[] header = new byte[sizeof(int)];
        NetResult readResult = await TryReadExactAsync(header, 0, header.Length, cancellation);

        if (readResult.IsFailed)
        {
            return NetResult<byte[]>.Fail(readResult.Error);
        }

        return NetResult<byte[]>.Success(header);
    }

    /// <summary>
    /// header에 연결되는 payload 전체 읽기
    /// </summary>
    private async Task<NetResult> AsyncReadPayload(byte[] packetBuffer, int payloadSize, CancellationToken cancellation)
    {
        NetResult readResult = await TryReadExactAsync(packetBuffer, sizeof(int), payloadSize, cancellation);
        if (readResult.IsFailed)
        {
            return readResult;
        }

        return NetResult.Success();
    }

    /// <summary>
    /// header 읽기에서는 정확히 4바이트가 찰 때까지 읽음
    /// payload 읽기에서는 size prefix에 적힌 payloadSize만큼 찰 때까지 읽음
    /// </summary>
    private async Task<NetResult> TryReadExactAsync(byte[] buffer, int offset, int count, CancellationToken cancellation)
    {
        try
        {
            int totalRead = 0;

            while (totalRead < count)
            {
                int read = await stream.ReadAsync(buffer, offset + totalRead, count - totalRead, cancellation);

                if (read == 0)
                {
                    return NetResult.Fail(new NetError(NetErrorType.Socket, "remote closed."));
                }

                totalRead += read;
            }

            return NetResult.Success();
        }
        catch (OperationCanceledException)
        {
            return NetResult.Fail(new NetError(NetErrorType.Canceled));
        }
        catch (IOException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Socket, ex.Message));
        }
        catch (ObjectDisposedException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Disposed, ex.Message));
        }
        catch (SocketException ex)
        {
            return NetResult.Fail(new NetError(NetErrorType.Socket, ex.Message));
        }
    }

    /// <summary>
    /// connection 종료
    /// </summary>
    public void Close()
    {
        strand.Post(() =>
        {
            // Write() 종료
            currentState.TryUnset(State.Writing);
            writeCancellation?.Cancel();

            // Read() 종료
            currentState.TryUnset(State.ReadEnabled);
            readCancellation?.Cancel();

            // 연결 종료
            currentState.TryUnset(State.Connected);
            try
            {
                socket.Close();
            }
            catch 
            {
            }
            OnClosed?.Invoke();

            return Task.CompletedTask;
        });
    }

    /// <summary>
    /// 수신 payload 단계 : flatbuffers 검증
    /// </summary>
    private bool TryValidateReceive(ref PacketContext context)
    {
        // 정상적인 size-prefixed packet?
        if (!TryValidatePacketBuffer(context.Buffer))
        {
            return false;
        }

        // TODO : 서버 - 클라 연결 상태가 안좋음
        // TODO : Rate limit, pending 제한 넣어야함

        return true;
    }

    /// <summary>
    /// 송신 packet 처리 queue에 enqueue
    /// </summary>
    private bool TryValidateSend(byte[] buffer)
    {
        // 정상적인 size-prefixed packet?
        if (!TryValidatePacketBuffer(buffer))
        {
            return false;
        }

        // 서버 - 클라 연결 상태가 안좋음
        if (sendQueue.Count >= SystemConfig.Connection.QueueSize)
        {
            return false;
        }

        return true;
    }

    /// <summary>
    /// size-prefixed flatbuffer packet 검증
    /// </summary>
    private static bool TryValidatePacketBuffer(byte[] buffer)
    {
        // size prefix 4바이트도 못 읽음?
        if (buffer.Length < sizeof(int))
        {
            return false;
        }

        // 내가 TCP에서 읽은 frame 길이와 size prefix가 정확히 일치?
        int payloadSize = BinaryPrimitives.ReadInt32LittleEndian(buffer);
        if (payloadSize <= 0 || buffer.Length != sizeof(int) + payloadSize)
        {
            return false;
        }

        // FlatBuffers 구조?
        var byteBuffer = new ByteBuffer(buffer);
        var verifier = new Verifier(byteBuffer);
        return verifier.VerifyBuffer("", true, PacketVerify.Verify);
    }
}
