using System.Threading;

namespace TRPG.Common;

/// <summary>
/// 1 씩 증가하는 CAS기반 ulong형 고유값 생성기
/// </summary>
public static class ConnectionIdGenerator
{
    /// <summary>
    /// 기본 connection id 값
    /// </summary>
    public const ulong DefaultId = 0;

    /// <summary>
    /// 마지막으로 발급된 connection id
    /// </summary>
    private static long lastConnectionId = 0;

    /// <summary>
    /// 다음 connection id 생성
    /// </summary>
    public static ulong Generate()
    {
        return (ulong)Interlocked.Increment(ref lastConnectionId);
    }
}
