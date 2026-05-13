# KTerm — Windows Terminal Rebuilt for KDE/Plasma

> **Current status (May 2026):** Phases 1–9 complete. The terminal runs, renders,
> and handles keyboard input. A tabbed `KXmlGuiWindow` with JSON-backed settings
> and multiple built-in color schemes is in place.
>
> **To run:** `cmake -S . -B build && cmake --build build && ./build/kterm/kterm`

This document describes the plan to port/rebuild Windows Terminal as a native
KDE/Plasma application on Linux (targeting Kubuntu 25.10+).

## Why Not Wine?

Wine has `windows.ui.xaml` stubs but no real XAML/WinUI 3 rendering. The WinRT
activation infrastructure, XAML Islands, Windows.UI.Composition, and ConPTY
are all unimplemented or non-functional in Wine as of 2026. Running Windows
Terminal under Wine is not a viable path.

## Completed Phases

| Phase | Component | Description | Status |
|-------|-----------|-------------|--------|
| 1 | Scaffolding | CMake root, ECM, Qt6+KF6 deps | ✅ Done |
| 2 | `kterm-vt` | VT state machine ported (`wchar_t→char32_t`, no Win32) | ✅ Done |
| 3 | `kterm-pty` | POSIX PTY (`openpty`/`forkpty` + `QSocketNotifier`) | ✅ Done |
| 4 | `kterm-core` | `TextBuffer`, `KTerminalDispatch`, `KTerminal` | ✅ Done |
| 5/6 | `kterm-widget` | `QPainter` cell renderer, keyboard, scrollback, cursor blink | ✅ Done |
| 7 | `kterm-settings` | JSON profiles + color schemes (`~/.config/kterm/settings.json`) | ✅ Done |
| 8/9 | `kterm` app | Tabbed `KXmlGuiWindow`, KDE actions, window title propagation | ✅ Done |

## Planned Phases

| Phase | Component | Description |
|-------|-----------|-------------|
| 10 | Qt RHI renderer | Glyph atlas + GLSL shaders replacing QPainter (optional optimisation) |
| 11 | Split panes | `QSplitter`-based pane management |
| 12 | Preferences UI | `KConfigDialog` for profiles + color schemes |
| 13 | KIO / SSH | Remote PTY connections via KIO |
| 14 | Flatpak/AppStream | Packaging and distribution |


## Architecture — Layer-by-Layer Mapping

| WT Layer                 | Technology          | KTerm Equivalent        | Technology         |
|--------------------------|---------------------|-------------------------|--------------------|
| `terminal/parser`        | C++20 (wchar_t)     | `lib/kterm-vt`          | C++20 (char32_t)   |
| `buffer/out`             | C++20 + Win32 types | `lib/kterm-vt`          | C++20              |
| `TerminalCore`           | C++/WinRT           | `lib/kterm-core`        | C++20 + Qt signals |
| `TerminalConnection`     | C++/WinRT + ConPTY  | `lib/kterm-pty`         | POSIX openpty      |
| `renderer/atlas`         | D3D11 + DirectWrite | `lib/kterm-render`      | Qt RHI + HarfBuzz  |
| `TerminalControl`        | XAML / WinRT        | `lib/kterm-widget`      | QML / Qt Quick     |
| `TerminalSettingsModel`  | JSON + WinRT        | `lib/kterm-settings`    | JSON + KConfig     |
| `TerminalApp`            | XAML / WinUI 3      | `kterm-app` (QML)       | Qt Quick           |
| `WindowsTerminal`        | Win32 + XAML Islands| `kterm`                 | KMainWindow        |

## Technology Stack

- **Language**: C++20
- **UI**: QML / Qt Quick 6
- **Framework**: KDE Frameworks 6 (KCoreAddons, KConfig, KWidgetsAddons, KWindowSystem, KXmlGui)
- **GPU rendering**: Qt RHI (OpenGL/Vulkan) with glyph atlas
- **Text shaping**: HarfBuzz + FreeType (via Qt internals or system libs)
- **PTY**: POSIX `openpty` / `forkpty` (`<pty.h>`)
- **Settings**: nlohmann/json + KConfig bridge
- **Build**: CMake 3.21+ + KDE Extra CMake Modules (ECM)
- **Packaging**: Flatpak / .deb

## Project Structure

```
kterm/                          (main window — KMainWindow)
lib/
  kterm-vt/                     (VT state machine + text buffer)
  kterm-pty/                    (POSIX PTY connections)
  kterm-core/                   (Terminal class: parser ↔ buffer ↔ input)
  kterm-render/                 (Qt RHI glyph atlas renderer)
  kterm-widget/                 (QML TerminalView item)
  kterm-settings/               (JSON profiles, color schemes, key bindings)
app/                            (tabs, panes, command palette — QML)
doc/kde-port/                   (this document + specs)
cmake/                          (CMake helper modules)
```

## Phased Implementation

### Phase 1 — Scaffolding ✅ (in progress)
- CMake root + per-library CMakeLists
- KDE app skeleton (KMainWindow, KAboutData)
- CI pipeline setup

### Phase 2 — VT Parser + Text Buffer (`lib/kterm-vt`)
Port `src/terminal/parser/stateMachine.{hpp,cpp}` and supporting types.
Key changes from Windows source:
- `wchar_t` → `char32_t` (UTF-32 code points instead of UTF-16)
- `std::wstring` → `std::u32string`
- `std::wstring_view` → `std::u32string_view`
- `BYTE` → `uint8_t`
- Remove `ParserTracing` (ETW/TraceLogging — Windows telemetry, not needed)
- Keep portable `til::enumset`, `til::small_vector` (zero Windows deps)
- Port `src/buffer/out/textBuffer`, `Row`, `TextAttribute`, `TextColor`

### Phase 3 — PTY Connection (`lib/kterm-pty`)
- `PtyConnection`: `openpty`/`forkpty`, async reads via `QSocketNotifier`
- `EchoConnection`: loopback for testing
- `ITerminalConnection` interface (connect/send/receive/resize/close)
- Shell detection from `$SHELL` env
- Resize: `TIOCSWINSZ` ioctl + `SIGWINCH`

### Phase 4 — Terminal Core (`lib/kterm-core`)
- `KTerminal` class (mirrors WT `Terminal.cpp`)
- Wire VT state machine → text buffer
- Input: Qt key events → VT byte sequences
- Selection model (linear + block)
- Scrollback management

### Phase 5 — GPU Renderer (`lib/kterm-render`)
- Qt RHI renderer (OpenGL/Vulkan backend)
- Glyph atlas (GPU texture, mirroring WT AtlasEngine/BackendD3D)
- HarfBuzz text shaping, FreeType rasterization
- GLSL shaders (vertex + fragment)
- Fallback: `QPainter` software renderer

### Phase 6 — Terminal Widget (`lib/kterm-widget`)
- `TerminalView` as `QQuickItem`
- Owns `KTerminal` + renderer + PTY connection
- Mouse/keyboard input routing
- Scrollbar, search overlay
- Accessibility (AT-SPI via `QAccessible`)

### Phase 7 — Settings Model (`lib/kterm-settings`)
- `settings.json` (compatible with WT format where practical)
- Profile inheritance: base layer → named profile → session override
- Color schemes (port WT defaults: Campbell, One Half, Solarized, Tango, Vintage)
- Action/key binding map
- KConfig bridge (respect KDE color scheme, DPI, cursor blink)

### Phase 8 — App Shell (`app/`)
- Tabs: QML `TabBar` with drag-reorder + close
- Panes: recursive binary-tree split (port WT `Pane.cpp` geometry logic)
- Command palette: QML overlay with fuzzy search
- Settings editor UI

### Phase 9 — Main Window & KDE Integration (`kterm/`)
- `KMainWindow` + `KAboutData`
- Quake/drop-down mode via `KWindowSystem`
- System tray (`KStatusNotifierItem`)
- Wayland support (Qt Wayland QPA)
- Multi-window IPC (`QLocalServer`/`QLocalSocket`)

### Phase 10 — Polish & Release
- SSH connection backend (`libssh2`)
- Flatpak manifest
- `vttest` VT conformance testing
- Accessibility audit
- Localization (KI18n)

## Build Dependencies (Kubuntu 25.10)

```
build-essential cmake ninja-build extra-cmake-modules
qt6-base-dev qt6-declarative-dev qt6-tools-dev qt6-wayland-dev
libkf6coreaddons-dev libkf6config-dev libkf6widgetsaddons-dev
libkf6windowsystem-dev libkf6notifications-dev libkf6xmlgui-dev
libkf6iconthemes-dev libkf6i18n-dev
libfreetype-dev libharfbuzz-dev libvulkan-dev libgl-dev
libutempter-dev nlohmann-json3-dev libgtest-dev
```
