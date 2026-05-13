# KTerm — KDE Plasma Terminal

> **Branch:** `kde-plasma-port`
>
> This is a KDE/Plasma native port of [Windows Terminal](https://github.com/microsoft/terminal),
> built from scratch using Qt 6 and KDE Frameworks 6. Windows-specific code
> (Win32, WinRT, XAML, ConPTY, MSIX) is removed as each layer is replaced with
> portable C++20 equivalents.

---

## Status

| Layer | Status | Notes |
|-------|--------|-------|
| VT parser (`kterm-vt`) | ✅ Done | Ported from `microsoft/terminal` — `wchar_t→char32_t`, no Win32 |
| PTY backend (`kterm-pty`) | ✅ Done | POSIX `openpty`/`forkpty` + `QSocketNotifier` |
| Terminal core (`kterm-core`) | ✅ Done | `TextBuffer`, `KTerminalDispatch` (CSI/SGR/OSC), `KTerminal` |
| Widget renderer (`kterm-widget`) | ✅ Done | `QPainter` cell renderer, keyboard input, scrollback, cursor blink |
| Settings model (`kterm-settings`) | ✅ Done | JSON profiles + color schemes (`~/.config/kterm/settings.json`) |
| App shell | ✅ Done | `KXmlGuiWindow` + `QTabWidget`, full KDE action/shortcut integration |
| Qt RHI GPU renderer | 🔲 Planned | Phase 11 — glyph atlas, GLSL shaders |
| QML/Kirigami UI | 🔲 Planned | Phase 12 — optional Wayland-native UI |
| SSH / sftp | 🔲 Planned | KIO-based remote connections |

---

## Building

### Prerequisites (Ubuntu 25.10 / Kubuntu)

```bash
sudo apt install \
    build-essential cmake ninja-build \
    extra-cmake-modules \
    qt6-base-dev qt6-base-dev-tools \
    libkf6coreaddons-dev libkf6i18n-dev libkf6widgetsaddons-dev \
    libkf6xmlgui-dev libkf6config-dev libkf6iconthemes-dev \
    libkf6notifications-dev libkf6windowsystem-dev
```

### Build

```bash
git clone https://github.com/Sannel/terminal.git kterm
cd kterm
git checkout kde-plasma-port

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

### Run

```bash
./build/kterm/kterm
```

---

## Architecture

```
kterm (executable)
├── lib/kterm-widget    — TerminalWidget (QPainter renderer, keyboard, scroll)
│   ├── lib/kterm-core  — KTerminal, TextBuffer, KTerminalDispatch
│   │   ├── lib/kterm-vt   — VT state machine (ported from microsoft/terminal)
│   │   └── lib/kterm-pty  — POSIX PTY via openpty + QSocketNotifier
│   └── lib/kterm-settings — JSON settings: profiles, color schemes
```

### Key classes

| Class | Location | Purpose |
|-------|----------|---------|
| `KTerm::StateMachine` | `kterm-vt` | VT/ANSI parser state machine |
| `KTerm::KTerminalDispatch` | `kterm-core` | Implements `IStateMachineEngine`; handles CSI, SGR, OSC |
| `KTerm::TextBuffer` | `kterm-core` | 2D cell grid + scrollback + cursor |
| `KTerm::KTerminal` | `kterm-core` | Owns PTY + state machine + buffer; emits Qt signals |
| `KTerm::TerminalWidget` | `kterm-widget` | `QAbstractScrollArea`; renders buffer, handles keyboard |
| `KTerm::KTermSettings` | `kterm-settings` | Singleton; loads/saves profiles and color schemes |
| `KTerm::MainWindow` | `kterm` app | `KXmlGuiWindow` with tabbed terminal interface |

---

## Settings

Settings are stored in `~/.config/kterm/settings.json`. The file is created
automatically on first run with defaults.

```json
{
  "defaultProfile": "Default",
  "profiles": {
    "Default": {
      "name": "Default",
      "shell": "",
      "workingDirectory": "",
      "font": "Monospace",
      "fontSize": 11,
      "colorScheme": "Default"
    }
  },
  "colorSchemes": {
    "Default": { ... },
    "One Dark":  { ... },
    "Solarized Dark": { ... }
  }
}
```

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+T` | New Tab |
| `Ctrl+Shift+W` | Close Tab |
| `Ctrl+Tab` | Next Tab |
| `Ctrl+Shift+Tab` | Previous Tab |
| `Ctrl+1`…`Ctrl+9` | Select Tab 1–9 |
| `Ctrl+C` | Send interrupt (SIGINT) to shell |
| `Ctrl+D` | Send EOF |
| `Shift+PgUp/PgDn` | Scroll scrollback |

---

## License

The VT parser code in `lib/kterm-vt` is derived from
[microsoft/terminal](https://github.com/microsoft/terminal) and carries the
original MIT license. All new KDE/Qt port code is © 2025 Sannel LLC, also
MIT-licensed.
