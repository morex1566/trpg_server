namespace TRPG.Networking;

public enum NetErrorType
{
    None,

    // 네트워크 상태 에러
    Canceled,
    Socket,
    Disposed,
    Timeout,
}

public readonly struct NetResult
{
    public bool IsSuccess => !Error.HasError;
    public bool IsFailed => Error.HasError;
    public NetError Error { get; }

    private NetResult(NetError error)
    {
        Error = error;
    }

    public static NetResult Success()
    {
        return new NetResult(NetError.None);
    }

    public static NetResult Fail(NetError error)
    {
        return new NetResult(error);
    }
}

public readonly struct NetResult<T>
{
    private readonly T? value;

    public bool IsSuccess => !Error.HasError;
    public bool IsFailed => Error.HasError;
    public NetError Error { get; }

    public T Value => value ?? throw new InvalidOperationException("result does not have value.");

    private NetResult(T value)
    {
        this.value = value;
        Error = NetError.None;
    }

    private NetResult(NetError error)
    {
        value = default;
        Error = error;
    }

    public static NetResult<T> Success(T value)
    {
        return new NetResult<T>(value);
    }

    public static NetResult<T> Fail(NetError error)
    {
        return new NetResult<T>(error);
    }
}

public readonly struct NetError
{
    public static readonly NetError None = new(NetErrorType.None, 0, string.Empty);
    public NetErrorType Type { get; } = NetErrorType.None;
    public int NativeCode { get; } = 0;
    public string Message { get; } = "";

    public bool HasError => Type != NetErrorType.None;

    public NetError(NetErrorType type, int nativeCode, string message)
    {
        Type = type;
        NativeCode = nativeCode;
        Message = message;
    }

    public NetError(NetErrorType type, string message)
    {
        Type = type;
        Message = message;
    }

    public NetError(NetErrorType type)
    {
        Type = type;
        Message = type.ToString();
    }

    public override string ToString()
    {
        if (!HasError) return "none";

        return NativeCode == 0 ? $"{Type}: {Message}" : $"{Type}({NativeCode}): {Message}";
    }
}
