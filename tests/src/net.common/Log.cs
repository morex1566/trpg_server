namespace Net.Common;

public sealed class Log : GlobalSingleton<Log>
{
    public Log()
    {
        Info($"create {Demangle(typeof(Log))} instance.");
    }

    public void Init() {}

    public static string Demangle(Type type)
    {
        return type.FullName ?? type.Name;
    }

    public static string Demangle(string name)
    {
        return name;
    }

    public void Info(string message)
    {
        Write("info", message);
    }

    public void Warn(string message)
    {
        Write("warn", message);
    }

    private static void Write(string level, string message)
    {
        Console.WriteLine($"[{DateTimeOffset.Now:yyyy-MM-dd HH:mm:ss.fff}] [{level}] {message}");
    }
}
