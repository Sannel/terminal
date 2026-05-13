// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "KTerminalDispatch.hpp"
#include "ascii.hpp"

#include <codecvt>
#include <locale>

namespace VT = Microsoft::Console::VirtualTerminal;

namespace KTerm
{
    KTerminalDispatch::KTerminalDispatch(TextBuffer& buffer) noexcept :
        _buffer(buffer)
    {
    }

    bool KTerminalDispatch::ActionExecute(const char32_t ch)
    {
        using AC = VT::AsciiChars;
        switch (ch)
        {
        case AC::BS:  _buffer.Backspace();       break;
        case AC::TAB: _buffer.Tab();              break;
        case AC::LF:
        case AC::VT:
        case AC::FF:  _buffer.LineFeed();         break;
        case AC::CR:  _buffer.CarriageReturn();   break;
        case AC::BEL: /* ring bell */              break;
        default:                                   break;
        }
        return true;
    }

    bool KTerminalDispatch::ActionExecuteFromEscape(const char32_t ch)
    {
        return ActionExecute(ch);
    }

    bool KTerminalDispatch::ActionPrint(const char32_t ch)
    {
        _buffer.PrintChar(ch);
        return true;
    }

    bool KTerminalDispatch::ActionPrintString(const std::u32string_view string)
    {
        _buffer.PrintString(string);
        return true;
    }

    bool KTerminalDispatch::ActionPassThroughString(const std::u32string_view string)
    {
        return ActionPrintString(string);
    }

    bool KTerminalDispatch::ActionEscDispatch(const VT::VTID id)
    {
        // ESC sequences (not C0, not CSI).
        switch (static_cast<uint64_t>(id))
        {
        case VT::VTID("M"): _buffer.ReverseLineFeed(); break; // ESC M = reverse index
        case VT::VTID("7"): _buffer.SaveCursor();       break; // ESC 7 = save cursor
        case VT::VTID("8"): _buffer.RestoreCursor();    break; // ESC 8 = restore cursor
        default:                                         break;
        }
        return true;
    }

    bool KTerminalDispatch::ActionVt52EscDispatch(const VT::VTID /*id*/,
                                                   const VT::VTParameters /*params*/)
    {
        return true; // VT52 sequences not needed for modern terminal emulation
    }

    bool KTerminalDispatch::ActionCsiDispatch(const VT::VTID id,
                                               const VT::VTParameters params)
    {
        const auto p1 = static_cast<int>(params.at(0).value_or(0));
        const auto p2 = static_cast<int>(params.at(1).value_or(0));

        switch (static_cast<uint64_t>(id))
        {
        // ── Cursor movement ─────────────────────────────────────────────
        case VT::VTID("A"): _buffer.CursorUp(std::max(1, p1));         break; // CUU
        case VT::VTID("B"): _buffer.CursorDown(std::max(1, p1));       break; // CUD
        case VT::VTID("C"): _buffer.CursorForward(std::max(1, p1));    break; // CUF
        case VT::VTID("D"): _buffer.CursorBackward(std::max(1, p1));   break; // CUB
        case VT::VTID("E"):  // CNL: cursor next line
            _buffer.CursorDown(std::max(1, p1));
            _buffer.CarriageReturn();
            break;
        case VT::VTID("F"):  // CPL: cursor prev line
            _buffer.CursorUp(std::max(1, p1));
            _buffer.CarriageReturn();
            break;
        case VT::VTID("G"): // CHA: cursor horizontal absolute
            _buffer.CursorToCol(std::max(1, p1) - 1);
            break;
        case VT::VTID("H"): // CUP: cursor position (1-based)
        case VT::VTID("f"): // HVP
            _buffer.CursorTo(std::max(1, p1) - 1, std::max(1, p2) - 1);
            break;
        case VT::VTID("d"): // VPA: vertical line position absolute
            _buffer.CursorToRow(std::max(1, p1) - 1);
            break;
        case VT::VTID("s"): _buffer.SaveCursor();    break; // SCP
        case VT::VTID("u"): _buffer.RestoreCursor(); break; // RCP

        // ── Erase ────────────────────────────────────────────────────────
        case VT::VTID("J"): _buffer.EraseInDisplay(p1); break; // ED
        case VT::VTID("K"): _buffer.EraseInLine(p1);    break; // EL
        case VT::VTID("X"): _buffer.EraseCharacters(std::max(1, p1)); break; // ECH

        // ── Insert / Delete ──────────────────────────────────────────────
        case VT::VTID("L"): _buffer.InsertLines(std::max(1, p1));   break; // IL
        case VT::VTID("M"): _buffer.DeleteLines(std::max(1, p1));   break; // DL
        case VT::VTID("@"): _buffer.InsertChars(std::max(1, p1));   break; // ICH
        case VT::VTID("P"): _buffer.DeleteChars(std::max(1, p1));   break; // DCH

        // ── Scroll ───────────────────────────────────────────────────────
        case VT::VTID("S"): _buffer.ScrollUp(std::max(1, p1));   break; // SU
        case VT::VTID("T"): _buffer.ScrollDown(std::max(1, p1)); break; // SD

        // ── Scroll region ────────────────────────────────────────────────
        case VT::VTID("r"): // DECSTBM
        {
            const int top    = p1 == 0 ? 1 : p1;
            const int bottom = p2 == 0 ? _buffer.Rows() : p2;
            _buffer.SetScrollRegion(top, bottom);
            break;
        }

        // ── SGR ──────────────────────────────────────────────────────────
        case VT::VTID("m"): _applySgr(params); break;

        // ── DA (device attributes) ────────────────────────────────────────
        // Handled at higher level; ignore here.
        default: break;
        }
        return true;
    }

    KTerminalDispatch::StringHandler
    KTerminalDispatch::ActionDcsDispatch(const VT::VTID /*id*/,
                                          const VT::VTParameters /*params*/)
    {
        // Return nullptr to ignore DCS strings (DECRQSS, sixel, etc.)
        return nullptr;
    }

    bool KTerminalDispatch::ActionOscDispatch(const size_t parameter,
                                               const std::u32string_view string)
    {
        if (parameter == 0 || parameter == 2)
        {
            // Window title — convert UTF-32 to UTF-8.
            _windowTitle.clear();
            for (const char32_t ch : string)
            {
                if (ch < 0x80)
                {
                    _windowTitle += static_cast<char>(ch);
                }
                else if (ch < 0x800)
                {
                    _windowTitle += static_cast<char>(0xC0 | (ch >> 6));
                    _windowTitle += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else if (ch < 0x10000)
                {
                    _windowTitle += static_cast<char>(0xE0 | (ch >> 12));
                    _windowTitle += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    _windowTitle += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else
                {
                    _windowTitle += static_cast<char>(0xF0 | (ch >> 18));
                    _windowTitle += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                    _windowTitle += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    _windowTitle += static_cast<char>(0x80 | (ch & 0x3F));
                }
            }
            if (_titleChangedCallback)
            {
                _titleChangedCallback(_windowTitle);
            }
        }
        return true;
    }

    bool KTerminalDispatch::ActionSs3Dispatch(const char32_t /*ch*/,
                                               const VT::VTParameters /*params*/)
    {
        return true;
    }

    void KTerminalDispatch::_applySgr(const VT::VTParameters params)
    {
        // Process SGR (Select Graphic Rendition) parameters.
        TextAttribute attr = _buffer.CurrentAttr();

        size_t i = 0;
        const size_t count = params.size();

        auto next = [&]() -> int {
            ++i;
            return static_cast<int>(params.at(i).value_or(0));
        };

        while (i < count)
        {
            const int p = static_cast<int>(params.at(i));
            switch (p)
            {
            case 0:  attr = TextAttribute{};    break; // reset
            case 1:  attr.bold          = true; break;
            case 2:  attr.faint         = true; break;
            case 3:  attr.italic        = true; break;
            case 4:  attr.underline     = true; break;
            case 5:  attr.blink         = true; break;
            case 7:  attr.inverse       = true; break;
            case 8:  attr.invisible     = true; break;
            case 9:  attr.strikethrough = true; break;
            case 22: attr.bold  = false; attr.faint = false; break;
            case 23: attr.italic        = false; break;
            case 24: attr.underline     = false; break;
            case 25: attr.blink         = false; break;
            case 27: attr.inverse       = false; break;
            case 28: attr.invisible     = false; break;
            case 29: attr.strikethrough = false; break;
            // Normal foreground colors (30-37, 90-97)
            case 30: case 31: case 32: case 33:
            case 34: case 35: case 36: case 37:
                attr.fg = TextColor::FromIndex(static_cast<uint8_t>(p - 30)); break;
            case 39: attr.fg = TextColor::Default(); break;
            case 40: case 41: case 42: case 43:
            case 44: case 45: case 46: case 47:
                attr.bg = TextColor::FromIndex(static_cast<uint8_t>(p - 40)); break;
            case 49: attr.bg = TextColor::Default(); break;
            case 90: case 91: case 92: case 93:
            case 94: case 95: case 96: case 97:
                attr.fg = TextColor::FromIndex(static_cast<uint8_t>(p - 90 + 8)); break;
            case 100: case 101: case 102: case 103:
            case 104: case 105: case 106: case 107:
                attr.bg = TextColor::FromIndex(static_cast<uint8_t>(p - 100 + 8)); break;
            // 256-color and RGB (38 / 48 with sub-params)
            case 38:
            case 48:
            {
                if (i + 1 < count)
                {
                    const int mode = next();
                    if (mode == 5 && i + 1 < count) // 256 color
                    {
                        const auto idx = static_cast<uint8_t>(next());
                        if (p == 38) { attr.fg = TextColor::From256(idx); }
                        else         { attr.bg = TextColor::From256(idx); }
                    }
                    else if (mode == 2 && i + 3 < count) // RGB
                    {
                        const auto r = static_cast<uint8_t>(next());
                        const auto g = static_cast<uint8_t>(next());
                        const auto b = static_cast<uint8_t>(next());
                        if (p == 38) { attr.fg = TextColor::FromRGB(r, g, b); }
                        else         { attr.bg = TextColor::FromRGB(r, g, b); }
                    }
                }
                break;
            }
            default: break;
            }
            ++i;
        }

        _buffer.SetAttr(attr);
    }
}
