using System;
using System.Threading;

namespace TRPG.Common;

/// <summary>
/// 서버 salt와 sequence를 조합해 guid를 생성
/// </summary>
public static class GuidGenerator
{
    /// <summary>
    /// sequence에 사용할 bit 수
    /// </summary>
    private const int SequenceBits = 48;

    /// <summary>
    /// 서버 프로세스별 salt
    /// </summary>
    private static readonly ulong serverSalt = CreateServerSalt();

    /// <summary>
    /// sequence counter
    /// </summary>
    private static long sequence = 0;

    /// <summary>
    /// 다음 guid 생성
    /// </summary>
    public static ulong Generate()
    {
        ulong currentSequence = (ulong)Interlocked.Increment(ref sequence) - 1;
        return (serverSalt << SequenceBits) | currentSequence;
    }

    /// <summary>
    /// 서버 프로세스별 salt 생성
    /// </summary>
    private static ulong CreateServerSalt()
    {
        return (ulong)Random.Shared.Next() & 0xFFFFUL;
    }
}
