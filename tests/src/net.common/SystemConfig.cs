namespace Net.Common;

public static class SystemConfig
{
    public const double LimitRamPercentage = 92.5;

    public static class Tcp
    {
        private const float TickRate = 15.0f;

        public const float TickIntervalMs = 1000.0f / TickRate;
    }

    public static class Connection
    {
        public const int BufferSizePerUser = 64 * 1024;
        public const int QueueSizePerUser = 1024;
        public const int BufferAlignment = 16;
    }

    public static class TcpAcceptTokenBucket
    {
        public const ulong Capacity = 100;
        public const double RefillIntervalMs = 10.0;
    }

    public static double CurrentRamPercentage()
    {
        GCMemoryInfo memoryInfo = GC.GetGCMemoryInfo();

        if (memoryInfo.TotalAvailableMemoryBytes <= 0) return 0.0;

        return 100.0 * memoryInfo.MemoryLoadBytes / memoryInfo.TotalAvailableMemoryBytes;
    }
}
