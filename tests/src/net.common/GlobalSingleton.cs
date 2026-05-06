using System.Reflection;

namespace Net.Common;

public abstract class GlobalSingleton<T> where T : class
{
    private static readonly Lazy<T> instance = new(CreateInstance, LazyThreadSafetyMode.ExecutionAndPublication);

    public static T GetInstance()
    {
        return instance.Value;
    }

    private static T CreateInstance()
    {
        T? created = Activator.CreateInstance
        (
            typeof(T),
            BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic,
            binder: null,
            args: [],
            culture: null
        ) as T;

        if (created is null)
        {
            throw new InvalidOperationException($"cannot create singleton instance: {typeof(T).FullName}");
        }

        return created;
    }
}
