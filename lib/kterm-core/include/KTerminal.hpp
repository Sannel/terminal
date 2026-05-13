// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "KTerminalDispatch.hpp"
#include "TextBuffer.hpp"
#include "PtyConnection.hpp"
#include "stateMachine.hpp"

#include <QObject>
#include <QString>

#include <memory>
#include <string>

namespace KTerm
{
    // The central terminal object: owns PTY + VT state machine + text buffer.
    // Connect to the repaintNeeded() signal to know when to redraw.
    class KTerminal : public QObject
    {
        Q_OBJECT
    public:
        explicit KTerminal(int rows = 24, int cols = 80, QObject* parent = nullptr);
        ~KTerminal() override;

        // Launch a shell (or other command) in the PTY.
        // Returns false if the PTY could not be started.
        bool Start(const std::string& command = "/bin/bash");

        // Send user input (keyboard / paste) to the PTY.
        void SendInput(std::string_view data);

        // Notify the terminal that the visible area was resized.
        void Resize(int rows, int cols);

        const TextBuffer& Buffer() const noexcept { return *_buffer; }

        int Rows() const noexcept { return _buffer->Rows(); }
        int Cols() const noexcept { return _buffer->Cols(); }

    signals:
        void repaintNeeded();
        void titleChanged(const QString& title);
        void terminated();

    private:
        std::unique_ptr<TextBuffer> _buffer;
        std::unique_ptr<Microsoft::Console::VirtualTerminal::StateMachine> _stateMachine;
        std::unique_ptr<PtyConnection> _pty;

        // Partial UTF-8 decode state.
        char32_t _utf8Codepoint = 0;
        int _utf8Remaining      = 0;
        std::u32string _utf32Buf;

        void _onPtyOutput(std::string_view data);
        void _decodeUtf8(unsigned char byte);
        void _flushUtf32();
    };
}
