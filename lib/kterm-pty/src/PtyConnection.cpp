// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "PtyConnection.hpp"

#include <QSocketNotifier>

#include <pty.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace KTerm
{
    PtyConnection::PtyConnection(QObject* parent) :
        QObject(parent)
    {
    }

    PtyConnection::~PtyConnection()
    {
        Close();
    }

    bool PtyConnection::Start(const std::string& command, unsigned short rows, unsigned short cols)
    {
        int masterFd = -1;
        int slaveFd = -1;

        if (openpty(&masterFd, &slaveFd, nullptr, nullptr, nullptr) < 0)
        {
            return false;
        }

        // Set initial window size.
        struct winsize ws{};
        ws.ws_row = rows;
        ws.ws_col = cols;
        ioctl(masterFd, TIOCSWINSZ, &ws);

        const pid_t pid = fork();
        if (pid < 0)
        {
            ::close(masterFd);
            ::close(slaveFd);
            return false;
        }

        if (pid == 0)
        {
            // Child process: set up PTY as controlling terminal.
            ::close(masterFd);
            setsid();
            ioctl(slaveFd, TIOCSCTTY, 0);
            dup2(slaveFd, STDIN_FILENO);
            dup2(slaveFd, STDOUT_FILENO);
            dup2(slaveFd, STDERR_FILENO);
            if (slaveFd > STDERR_FILENO)
            {
                ::close(slaveFd);
            }
            const char* shell = command.empty() ? nullptr : command.c_str();
            if (!shell) {
                shell = getenv("SHELL");
            }
            if (!shell || shell[0] == '\0') {
                shell = "/bin/bash";
            }
            execl(shell, shell, nullptr);
            _exit(1);
        }

        // Parent process.
        ::close(slaveFd);
        _masterFd = masterFd;
        _childPid = pid;

        _notifier = std::make_unique<QSocketNotifier>(masterFd, QSocketNotifier::Read, this);
        connect(_notifier.get(), &QSocketNotifier::activated,
                this, &PtyConnection::_onMasterReadable);

        return true;
    }

    void PtyConnection::_onMasterReadable(QSocketDescriptor /*fd*/, QSocketNotifier::Type /*type*/)
    {
        char buf[4096];
        const ssize_t n = ::read(_masterFd, buf, sizeof(buf));
        if (n > 0)
        {
            if (_outputCallback)
            {
                _outputCallback(std::string_view(buf, static_cast<size_t>(n)));
            }
        }
        else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EINTR))
        {
            // EOF or error: child process has exited.
            _notifier.reset();
            if (_terminatedCallback)
            {
                _terminatedCallback();
            }
        }
    }

    void PtyConnection::WriteInput(std::string_view data)
    {
        if (_masterFd < 0)
        {
            return;
        }
        const char* ptr = data.data();
        size_t remaining = data.size();
        while (remaining > 0)
        {
            const ssize_t n = ::write(_masterFd, ptr, remaining);
            if (n <= 0)
            {
                break;
            }
            ptr += n;
            remaining -= static_cast<size_t>(n);
        }
    }

    void PtyConnection::Resize(unsigned short rows, unsigned short cols)
    {
        if (_masterFd < 0)
        {
            return;
        }
        struct winsize ws{};
        ws.ws_row = rows;
        ws.ws_col = cols;
        ioctl(_masterFd, TIOCSWINSZ, &ws);
    }

    void PtyConnection::Close()
    {
        if (_notifier)
        {
            _notifier.reset();
        }
        if (_masterFd >= 0)
        {
            ::close(_masterFd);
            _masterFd = -1;
        }
        if (_childPid > 0)
        {
            int status = 0;
            waitpid(_childPid, &status, WNOHANG);
            _childPid = -1;
        }
    }

    void PtyConnection::SetOutputCallback(OutputCallback callback)
    {
        _outputCallback = std::move(callback);
    }

    void PtyConnection::SetTerminatedCallback(TerminatedCallback callback)
    {
        _terminatedCallback = std::move(callback);
    }
}
