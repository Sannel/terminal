// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "ITerminalConnection.hpp"

#include <QObject>
#include <QSocketNotifier>

#include <memory>
#include <string>

namespace KTerm
{
    // A terminal connection backed by a POSIX PTY (openpty/forkpty).
    // The child process (shell) runs in the PTY slave, while we read/write
    // the master fd via a QSocketNotifier for Qt-integrated async I/O.
    class PtyConnection : public QObject, public ITerminalConnection
    {
        Q_OBJECT
    public:
        explicit PtyConnection(QObject* parent = nullptr);
        ~PtyConnection() override;

        // Launch the specified command (e.g. "/bin/bash") in the PTY.
        // Returns true on success.
        bool Start(const std::string& command, unsigned short rows, unsigned short cols);

        // ITerminalConnection
        void WriteInput(std::string_view data) override;
        void Resize(unsigned short rows, unsigned short cols) override;
        void Close() override;
        void SetOutputCallback(OutputCallback callback) override;
        void SetTerminatedCallback(TerminatedCallback callback) override;

    private slots:
        void _onMasterReadable(QSocketDescriptor fd, QSocketNotifier::Type type);

    private:
        int _masterFd = -1;
        pid_t _childPid = -1;
        std::unique_ptr<QSocketNotifier> _notifier;
        OutputCallback _outputCallback;
        TerminatedCallback _terminatedCallback;
    };
}
