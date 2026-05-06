using System.Diagnostics;

namespace Net.Common;

public sealed class Time : GlobalSingleton<Time>
{
    public enum TimeUnitType
    {
        Second,
        Millisecond
    }

    private readonly Stopwatch stopwatch = Stopwatch.StartNew();
    private TimeSpan lastTimestamp = TimeSpan.Zero;
    private TimeSpan deltaTime = TimeSpan.Zero;

    public Time()
    {
        Log.Temp($"create {Log.Demangle(typeof(Time))} instance.");
    }

    public float Deltatime(TimeUnitType unit = TimeUnitType.Second)
    {
        return unit switch
        {
            TimeUnitType.Millisecond => (float)deltaTime.TotalMilliseconds,
            _ => (float)deltaTime.TotalSeconds
        };
    }

    public ulong Timestamp(TimeUnitType unit = TimeUnitType.Millisecond)
    {
        TimeSpan duration = stopwatch.Elapsed;

        return unit switch
        {
            TimeUnitType.Second => (ulong)duration.TotalSeconds,
            _ => (ulong)duration.TotalMilliseconds
        };
    }

    public void Update()
    {
        TimeSpan now = stopwatch.Elapsed;
        deltaTime = now - lastTimestamp;
        lastTimestamp = now;
    }
}
