// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "TextBuffer.hpp"

#include "IStateMachineEngine.hpp"

#include <cstdint>
#include <string>

namespace KTerm
{
    // Implements the VT state machine engine interface to drive a TextBuffer.
    // This is the terminal emulator's "dispatch" layer — it interprets VT escape
    // sequences and translates them into TextBuffer operations.
    class KTerminalDispatch : public Microsoft::Console::VirtualTerminal::IStateMachineEngine
    {
    public:
        explicit KTerminalDispatch(TextBuffer& buffer) noexcept;

        // ── IStateMachineEngine ───────────────────────────────────────────
        bool ActionExecute(const char32_t ch) override;
        bool ActionExecuteFromEscape(const char32_t ch) override;
        bool ActionPrint(const char32_t ch) override;
        bool ActionPrintString(const std::u32string_view string) override;
        bool ActionPassThroughString(const std::u32string_view string) override;
        bool ActionEscDispatch(const Microsoft::Console::VirtualTerminal::VTID id) override;
        bool ActionVt52EscDispatch(const Microsoft::Console::VirtualTerminal::VTID id,
                                   const Microsoft::Console::VirtualTerminal::VTParameters parameters) override;
        bool ActionCsiDispatch(const Microsoft::Console::VirtualTerminal::VTID id,
                               const Microsoft::Console::VirtualTerminal::VTParameters parameters) override;
        StringHandler ActionDcsDispatch(const Microsoft::Console::VirtualTerminal::VTID id,
                                        const Microsoft::Console::VirtualTerminal::VTParameters parameters) override;
        bool ActionOscDispatch(const size_t parameter, const std::u32string_view string) override;
        bool ActionSs3Dispatch(const char32_t ch,
                               const Microsoft::Console::VirtualTerminal::VTParameters parameters) override;
        void UnknownSequence() noexcept override {}
        bool EncounteredWin32InputModeSequence() const noexcept override { return false; }

        // Window title set via OSC 0/2.
        const std::string& WindowTitle() const noexcept { return _windowTitle; }

        using TitleChangedCallback = std::function<void(const std::string&)>;
        void SetTitleChangedCallback(TitleChangedCallback cb) { _titleChangedCallback = std::move(cb); }

    private:
        TextBuffer& _buffer;
        TextAttribute _pendingAttr;
        std::string _windowTitle;
        TitleChangedCallback _titleChangedCallback;

        void _applySgr(const Microsoft::Console::VirtualTerminal::VTParameters params);
    };
}
