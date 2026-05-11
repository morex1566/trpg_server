using Terminal.Gui;

public sealed class Shell
{
    private readonly List<string> messages = new();

    private readonly List<string> suggestions = new();

    private readonly List<string> commands = new();

    private ListView? messageList = null;

    private ListView? suggestionList = null;

    private TextField? inputField = null;

    public Func<string, Task>? Submitted;

    public void SetCommands(IEnumerable<string> commands)
    {
        this.commands.Clear();
        this.commands.AddRange(commands);
    }

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
            Height = Dim.Fill() - 8,
            ColorScheme = shellScheme
        };

        suggestionList = new ListView(suggestions)
        {
            X = 1,
            Y = Pos.Bottom(messageList),
            Width = Dim.Fill() - 2,
            Height = 4,
            ColorScheme = shellScheme,
            Visible = false
        };

        inputField = new TextField("")
        {
            X = 1,
            Y = Pos.Bottom(suggestionList),
            Width = Dim.Fill() - 12,
            ColorScheme = shellScheme
        };

        Button submitButton = new Button("Send")
        {
            X = Pos.Right(inputField) + 1,
            Y = Pos.Top(inputField),
            ColorScheme = shellScheme
        };

        submitButton.Clicked += () =>
        {
            _ = SubmitAsync();
        };

        inputField.TextChanged += _ =>
        {
            UpdateSuggestions();
        };

        inputField.KeyPress += key =>
        {
            if (key.KeyEvent.Key == Key.Enter)
            {
                key.Handled = true;
                _ = SubmitAsync();
                return;
            }

            if (key.KeyEvent.Key == Key.Tab)
            {
                key.Handled = true;
                ApplySuggestion();
                return;
            }

            if (key.KeyEvent.Key == Key.CursorDown)
            {
                key.Handled = true;
                MoveSuggestion(1);
                return;
            }

            if (key.KeyEvent.Key == Key.CursorUp)
            {
                key.Handled = true;
                MoveSuggestion(-1);
            }
        };

        MenuBar menuBar = new MenuBar(new MenuBarItem[]
        {
            new MenuBarItem("_File", new MenuItem[]
            {
                new MenuItem("_Quit", "", () =>
                {
                    Stop();
                })
            })
        })
        {
            ColorScheme = menuScheme
        };

        mainWindow.Add(messageList, suggestionList, inputField, submitButton);
        Application.Top.Add(menuBar, mainWindow);

        Append("Ready. Type /help.");

        Application.Run();
        Application.Shutdown();
    }

    public void Stop()
    {
        Application.MainLoop.Invoke(() =>
        {
            Application.RequestStop();
        });
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

    public void Clear()
    {
        if (messageList == null)
        {
            messages.Clear();
            return;
        }

        Application.MainLoop.Invoke(() =>
        {
            messages.Clear();
            messageList.SetSource(messages);
        });
    }

    private async Task SubmitAsync()
    {
        if (inputField == null) return;

        string text = inputField.Text?.ToString() ?? string.Empty;
        if (string.IsNullOrWhiteSpace(text)) return;

        inputField.Text = "";
        HideSuggestions();

        Append($"> {text}");

        if (Submitted == null) return;

        await Submitted.Invoke(text);
    }

    private void UpdateSuggestions()
    {
        if (inputField == null || suggestionList == null) return;

        string text = inputField.Text?.ToString() ?? string.Empty;
        if (!text.StartsWith('/'))
        {
            HideSuggestions();
            return;
        }

        string commandPrefix = text.Split(' ', StringSplitOptions.RemoveEmptyEntries).FirstOrDefault() ?? text;

        suggestions.Clear();
        suggestions.AddRange
        (
            commands
                .Where(command => command.StartsWith(commandPrefix, StringComparison.OrdinalIgnoreCase))
                .OrderBy(command => command)
        );

        suggestionList.SetSource(suggestions);
        suggestionList.SelectedItem = suggestions.Count > 0 ? 0 : -1;
        suggestionList.Visible = suggestions.Count > 0;
        suggestionList.SetNeedsDisplay();
    }

    private void ApplySuggestion()
    {
        if (inputField == null || suggestionList == null || suggestions.Count == 0) return;

        int index = Math.Clamp(suggestionList.SelectedItem, 0, suggestions.Count - 1);
        string selected = suggestions[index];
        string commandName = selected.Split(' ', StringSplitOptions.RemoveEmptyEntries)[0];

        inputField.Text = commandName + " ";
        inputField.CursorPosition = inputField.Text.Length;
        HideSuggestions();
    }

    private void MoveSuggestion(int delta)
    {
        if (suggestionList == null || suggestions.Count == 0) return;

        int nextIndex = suggestionList.SelectedItem + delta;
        nextIndex = Math.Clamp(nextIndex, 0, suggestions.Count - 1);
        suggestionList.SelectedItem = nextIndex;
    }

    private void HideSuggestions()
    {
        if (suggestionList == null) return;

        suggestions.Clear();
        suggestionList.SetSource(suggestions);
        suggestionList.Visible = false;
        suggestionList.SetNeedsDisplay();
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
