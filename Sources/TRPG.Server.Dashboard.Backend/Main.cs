namespace TRPG.Server.Dashboard.Backend;

static class Program
{
    static void Main(string[] args)
    {
        var builder = WebApplication.CreateBuilder(args);
        var app = builder.Build();

        app.MapGet("/health", () => Results.Ok(new
        {
            status = "ok"
        }));

        app.Run();
    }
}
