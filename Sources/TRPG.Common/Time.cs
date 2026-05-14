using System;
using System.Diagnostics;

namespace TRPG.Common;

/// <summary>
/// 서버 시간 및 delta time 계산 담당
/// </summary>
public sealed class Time
{
    /// <summary>
    /// 시간 단위
    /// </summary>
    public enum TimeUnitType
    {
        Second,
        Millisecond
    }


    /// <summary>
    /// time 클래스가 생성된 시간
    /// </summary>
    private readonly long startTicks;

    /// <summary>
    /// 마지막으로 Update()가 호출된 시간
    /// </summary>
    private long lastTicks;

    /// <summary>
    /// 마지막 Update()에서 계산된 프레임 단위 시간
    /// </summary>
    private long deltaTicks;


    /// <summary>
    /// 전역 싱글톤 인스턴스
    /// </summary>
    public static Time Instance { get; } = new Time();


    /// <summary>
    /// Time 인스턴스 생성
    /// </summary>
    private Time()
    {
        startTicks = Stopwatch.GetTimestamp();
        lastTicks = startTicks;
        deltaTicks = 0;
    }


    /// <summary>
    /// current timestamp - last timestamp
    /// </summary>
    public float DeltaTime(TimeUnitType unit = TimeUnitType.Second)
    {
        double seconds = (double)deltaTicks / Stopwatch.Frequency;

        return unit switch
        {
            TimeUnitType.Millisecond => (float)(seconds * 1000.0),
            _ => (float)seconds
        };
    }

    /// <summary>
    /// start_timestamp 기준 현재 시간
    /// </summary>
    public ulong Timestamp(TimeUnitType unit = TimeUnitType.Millisecond)
    {
        long now = Stopwatch.GetTimestamp();
        double seconds = (double)(now - startTicks) / Stopwatch.Frequency;

        return unit switch
        {
            TimeUnitType.Second => (ulong)seconds,
            _ => (ulong)(seconds * 1000.0)
        };
    }

    /// <summary>
    /// last_timestamp를 최신화
    /// </summary>
    public void Update()
    {
        long now = Stopwatch.GetTimestamp();
        deltaTicks = now - lastTicks;
        lastTicks = now;
    }
}
