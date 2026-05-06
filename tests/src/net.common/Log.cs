namespace Net.Common;

public sealed class Log : GlobalSingleton<Log>
{
    private static readonly object consoleLock = new();

    public Log()
    {
        Info($"create {Demangle(typeof(Log))} instance.");
    }

    public void Init() { }

    public static string Demangle(Type type)
    {
        return type.FullName ?? type.Name;
    }

    public static string Demangle(string name)
    {
        return name;
    }

    public void Temp(string message)
    {
        Write("Temp", message, ConsoleColor.White);
    }

    public void Warn(string message)
    {
        Write("Warn", message, ConsoleColor.Yellow);
    }

    public void Error(string message)
    {
        Write("Error", message, ConsoleColor.Red);
    }

    private static void Write(string level, string message, ConsoleColor color)
    {
        lock (consoleLock)
        {
            ConsoleColor prevColor = Console.ForegroundColor;

            Console.ForegroundColor = color;
            Console.WriteLine($"[{DateTimeOffset.Now:yyyy-MM-dd HH:mm:ss.fff}] [{level}] {message}");
            Console.ForegroundColor = prevColor;
        }
    }
}