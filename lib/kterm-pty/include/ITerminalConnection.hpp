// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <functional>
#include <string_view>

namespace KTerm
{
    // Interface implemented by each terminal connection backend (PTY, echo, SSH, …).
    class ITerminalConnection
    {
    public:
        virtual ~ITerminalConnection() = default;

        // Write data from the application process to the terminal.
        virtual void WriteInput(std::string_view data) = 0;

        // Notify the connection of a terminal resize.
        virtual void Resize(unsigned short rows, unsigned short cols) = 0;

        // Gracefully close the connection.
        virtual void Close() = 0;

        // Called when the connection receives output from the application process.
        using OutputCallback = std::function<void(std::string_view)>;
        virtual void SetOutputCallback(OutputCallback callback) = 0;

        // Called when the connection terminates (process exits, SSH disconnect, …).
        using TerminatedCallback = std::function<void()>;
        virtual void SetTerminatedCallback(TerminatedCallback callback) = 0;
    };
}
