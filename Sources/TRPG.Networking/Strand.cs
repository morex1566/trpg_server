using System;
using System.Threading;
using System.Threading.Channels;
using System.Threading.Tasks;

namespace TRPG.Networking;

/// <summary>
/// boost::asio::strand 대응. Channel 기반 단일 consumer로 직렬 실행 보장
/// </summary>
public sealed class Strand : IAsyncDisposable
{
    /// <summary>
    /// 작업 큐 채널
    /// </summary>
    private readonly Channel<Func<Task>> channel;

    /// <summary>
    /// consumer loop task
    /// </summary>
    private readonly Task loopTask;

    /// <summary>
    /// 취소 토큰
    /// </summary>
    private readonly CancellationTokenSource cancellation;


    /// <summary>
    /// strand 인스턴스 생성
    /// </summary>
    public Strand()
    {
        channel = Channel.CreateUnbounded<Func<Task>>(new UnboundedChannelOptions
        {
            SingleReader = true,
            SingleWriter = false
        });

        cancellation = new CancellationTokenSource();
        loopTask = Task.Run(() => ConsumerLoopAsync(cancellation.Token));
    }


    /// <summary>
    /// strand에 작업 post
    /// </summary>
    public void Post(Func<Task> work)
    {
        channel.Writer.TryWrite(work);
    }

    /// <summary>
    /// strand에 작업 post
    /// </summary>
    public Task<T> Post<T>(Func<T> work)
    {
        var tcs = new TaskCompletionSource<T>(TaskCreationOptions.RunContinuationsAsynchronously);

        bool written = channel.Writer.TryWrite(() =>
        {
            try
            {
                T result = work();
                tcs.SetResult(result);
            }
            catch (Exception ex)
            {
                tcs.SetException(ex);
            }

            return Task.CompletedTask;
        });

        if (!written)
        {
            tcs.SetException(new InvalidOperationException("Failed to post work to strand."));
        }

        return tcs.Task;
    }

    /// <summary>
    /// 단일 consumer loop. 큐에 들어온 작업을 순차적으로 실행
    /// </summary>
    private async Task ConsumerLoopAsync(CancellationToken ct)
    {
        try
        {
            await foreach (var work in channel.Reader.ReadAllAsync(ct))
            {
                try
                {
                    await work();
                }
                catch (Exception ex)
                {
                    Serilog.Log.Error(ex, "strand work error.");
                }
            }
        }
        catch (OperationCanceledException)
        {
            // 정상 종료
        }
    }

    public async ValueTask DisposeAsync()
    {
        channel.Writer.Complete();
        cancellation.Cancel();

        try
        {
            await loopTask;
        }
        catch (OperationCanceledException) {}

        cancellation.Dispose();
    }
}
