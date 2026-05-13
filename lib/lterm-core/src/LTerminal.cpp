// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "LTerminal.hpp"

#include <QString>

namespace LTerm
{
    LTerminal::LTerminal(int rows, int cols, QObject* parent) :
        QObject(parent),
        _buffer(std::make_unique<TextBuffer>(rows, cols))
    {
        _buffer->SetDirtyCallback([this]() {
            emit repaintNeeded();
        });

        // Create dispatch, wire title callback, then move ownership to state machine.
        auto dispatch = std::make_unique<LTerminalDispatch>(*_buffer);
        dispatch->SetTitleChangedCallback([this](const std::string& title) {
            emit titleChanged(QString::fromStdString(title));
        });

        _stateMachine = std::make_unique<Microsoft::Console::VirtualTerminal::StateMachine>(
            std::move(dispatch), false);

        // Create PTY.
        _pty = std::make_unique<PtyConnection>(this);

        _pty->SetOutputCallback([this](std::string_view data) {
            _onPtyOutput(data);
        });

        _pty->SetTerminatedCallback([this]() {
            emit terminated();
        });
    }

    LTerminal::~LTerminal()
    {
        _pty->Close();
    }

    bool LTerminal::Start(const std::string& command, const std::string& workingDir)
    {
        return _pty->Start(command,
                           static_cast<unsigned short>(_buffer->Rows()),
                           static_cast<unsigned short>(_buffer->Cols()),
                           workingDir);
    }

    void LTerminal::SendInput(std::string_view data)
    {
        _pty->WriteInput(data);
    }

    void LTerminal::Resize(int rows, int cols)
    {
        _buffer->Resize(rows, cols);
        _pty->Resize(static_cast<unsigned short>(rows),
                     static_cast<unsigned short>(cols));
    }

    void LTerminal::_onPtyOutput(std::string_view data)
    {
        for (const auto byte : data)
        {
            _decodeUtf8(static_cast<unsigned char>(byte));
        }
        _flushUtf32();
    }

    void LTerminal::_decodeUtf8(unsigned char byte)
    {
        if (_utf8Remaining > 0)
        {
            // Continuation byte.
            if ((byte & 0xC0) == 0x80)
            {
                _utf8Codepoint = (_utf8Codepoint << 6) | (byte & 0x3F);
                --_utf8Remaining;
                if (_utf8Remaining == 0)
                {
                    _utf32Buf += _utf8Codepoint;
                }
                return;
            }
            // Invalid continuation — reset and re-process as new sequence.
            _utf8Remaining = 0;
        }

        // Start of a new codepoint.
        if (byte < 0x80)
        {
            _utf32Buf += static_cast<char32_t>(byte);
        }
        else if ((byte & 0xE0) == 0xC0)
        {
            _utf8Codepoint = byte & 0x1F;
            _utf8Remaining = 1;
        }
        else if ((byte & 0xF0) == 0xE0)
        {
            _utf8Codepoint = byte & 0x0F;
            _utf8Remaining = 2;
        }
        else if ((byte & 0xF8) == 0xF0)
        {
            _utf8Codepoint = byte & 0x07;
            _utf8Remaining = 3;
        }
        // else: invalid lead byte, skip it
    }

    void LTerminal::_flushUtf32()
    {
        if (!_utf32Buf.empty())
        {
            _stateMachine->ProcessString(_utf32Buf);
            _utf32Buf.clear();
        }
    }
}
