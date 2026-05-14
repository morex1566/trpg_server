using System;
using System.Threading;

namespace TRPG.Common;

/// <summary>
/// CAS 기반 token bucket rate limiter
/// </summary>
public sealed class TokenBucket
{
    /// <summary>
    /// 버킷 최대 사이즈
    /// </summary>
    private readonly ulong capacity;

    /// <summary>
    /// 토큰 1개가 회복되는 간격(ms)
    /// </summary>
    private readonly double refillIntervalMs;

    /// <summary>
    /// 현재 token 수 (Interlocked용)
    /// </summary>
    private long stateTokens;

    /// <summary>
    /// 마지막 token 갱신 시간(ms) (Interlocked용)
    /// </summary>
    private long stateLastMs;


    /// <summary>
    /// token bucket 생성
    /// </summary>
    public TokenBucket(ulong capacity, double refillIntervalMs)
    {
        this.capacity = capacity;
        this.refillIntervalMs = refillIntervalMs;

        stateTokens = (long)capacity;
        stateLastMs = (long)Time.Instance.Timestamp();
    }


    /// <summary>
    /// 지정한 token 수만큼 소모 시도
    /// </summary>
    public bool Consume(ulong amount = 1)
    {
        // CAS 루프
        while (true)
        {
            long currentTokens = Interlocked.Read(ref stateTokens);
            long currentLastMs = Interlocked.Read(ref stateLastMs);

            ulong currentMs = Time.Instance.Timestamp();
            ulong elapsedMs = currentMs - (ulong)currentLastMs;

            // 지난 시간만큼 token 추가
            ulong tokens = (ulong)Math.Min((long)capacity, currentTokens);
            ulong generated = (ulong)(elapsedMs / refillIntervalMs);
            ulong availableTokens = (generated >= capacity - tokens)
                ? capacity
                : tokens + generated;

            // 토큰 없다... 클라 연결 거부
            if (availableTokens < amount) return false;

            // 토큰 있다... 토큰 소모하고 정보 최신화
            long desiredTokens = (long)(availableTokens - amount);
            long desiredLastMs = (generated > 0)
                ? currentLastMs + (long)(generated * refillIntervalMs)
                : currentLastMs;

            if (Interlocked.CompareExchange(ref stateTokens, desiredTokens, currentTokens) == currentTokens)
            {
                Interlocked.CompareExchange(ref stateLastMs, desiredLastMs, currentLastMs);
                return true;
            }
            // CAS 실패 → 재시도
        }
    }
}
