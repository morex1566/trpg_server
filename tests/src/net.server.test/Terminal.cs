using Terminal.Gui;

public class ShellView
{
    private readonly List<string> messages = new();

    private ListView? messageList = null;

    private TextField? inputField = null;

    public event Action<string>? Submitted;

    public void Run()
    {
        Application.Init();
        ApplyTheme();

        ColorScheme shellScheme = Colors.Base;
        ColorScheme menuScheme = Colors.Menu;

        Window mainWindow = new Window("TRPG Server Test")
        {
            X = 0,
            Y = 1,
            Width = Dim.Fill(),
            Height = Dim.Fill(),
            ColorScheme = shellScheme
        };

        messageList = new ListView(messages)
        {
            X = 1,
            Y = 1,
            Width = Dim.Fill() - 2,
            Height = Dim.Fill() - 4,
            ColorScheme = shellScheme
        };

        inputField = new TextField("")
        {
            X = 1,
            Y = Pos.Bottom(messageList) + 1,
            Width = Dim.Fill() - 12,
            ColorScheme = shellScheme
        };

        Button submitButton = new Button("Send")
        {
            X = Pos.Right(inputField) + 1,
            Y = Pos.Top(inputField),
            ColorScheme = shellScheme
        };

        submitButton.Clicked += Submit;

        inputField.KeyPress += key =>
        {
            if (key.KeyEvent.Key != Key.Enter) return;

            key.Handled = true;
            Submit();
        };

        MenuBar menuBar = new MenuBar(new MenuBarItem[]
        {
            new MenuBarItem("_File", new MenuItem[]
            {
                new MenuItem("_Quit", "", () =>
                {
                    Application.RequestStop();
                })
            })
        })
        {
            ColorScheme = menuScheme
        };

        mainWindow.Add(messageList, inputField, submitButton);
        Application.Top.Add(menuBar, mainWindow);

        Append("Ready.");

        Application.Run();
        Application.Shutdown();
    }

    public void Append(string message)
    {
        if (messageList == null)
        {
            messages.Add(message);
            return;
        }

        Application.MainLoop.Invoke(() =>
        {
            messages.Add(message);
            messageList.SetSource(messages);
            messageList.SelectedItem = Math.Max(0, messages.Count - 1);
        });
    }

    private void Submit()
    {
        if (inputField == null) return;

        string text = inputField.Text?.ToString() ?? string.Empty;
        if (string.IsNullOrWhiteSpace(text)) return;

        inputField.Text = "";

        Append($"> {text}");
        Submitted?.Invoke(text);
    }

    private static void ApplyTheme()
    {
        ColorScheme shellScheme = new ColorScheme()
        {
            Normal = Terminal.Gui.Attribute.Make(Color.Gray, Color.Black),
            Focus = Terminal.Gui.Attribute.Make(Color.White, Color.DarkGray),
            HotNormal = Terminal.Gui.Attribute.Make(Color.BrightGreen, Color.Black),
            HotFocus = Terminal.Gui.Attribute.Make(Color.White, Color.DarkGray),
            Disabled = Terminal.Gui.Attribute.Make(Color.DarkGray, Color.Black)
        };

        ColorScheme menuScheme = new ColorScheme()
        {
            Normal = Terminal.Gui.Attribute.Make(Color.Gray, Color.Black),
            Focus = Terminal.Gui.Attribute.Make(Color.Black, Color.Gray),
            HotNormal = Terminal.Gui.Attribute.Make(Color.BrightGreen, Color.Black),
            HotFocus = Terminal.Gui.Attribute.Make(Color.Black, Color.Gray),
            Disabled = Terminal.Gui.Attribute.Make(Color.DarkGray, Color.Black)
        };

        Colors.Base = shellScheme;
        Colors.TopLevel = shellScheme;
        Colors.Dialog = shellScheme;
        Colors.Menu = menuScheme;
    }
}
