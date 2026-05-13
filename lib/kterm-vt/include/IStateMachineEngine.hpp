// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// KTerm port: adapted for Linux/KDE — wchar_t → char32_t, removed Win32 types.

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>

#include "DispatchTypes.hpp"

namespace Microsoft::Console::VirtualTerminal
{
    class IStateMachineEngine
    {
    public:
        using StringHandler = std::function<bool(const char32_t)>;

        virtual ~IStateMachineEngine() = 0;
        IStateMachineEngine(const IStateMachineEngine&) = default;
        IStateMachineEngine(IStateMachineEngine&&) = default;
        IStateMachineEngine& operator=(const IStateMachineEngine&) = default;
        IStateMachineEngine& operator=(IStateMachineEngine&&) = default;

        virtual void UnknownSequence() noexcept = 0;
        virtual bool EncounteredWin32InputModeSequence() const noexcept = 0;

        virtual bool ActionExecute(const char32_t ch) = 0;
        virtual bool ActionExecuteFromEscape(const char32_t ch) = 0;
        virtual bool ActionPrint(const char32_t ch) = 0;
        virtual bool ActionPrintString(const std::u32string_view string) = 0;

        virtual bool ActionPassThroughString(const std::u32string_view string) = 0;

        virtual bool ActionEscDispatch(const VTID id) = 0;
        virtual bool ActionVt52EscDispatch(const VTID id, const VTParameters parameters) = 0;
        virtual bool ActionCsiDispatch(const VTID id, const VTParameters parameters) = 0;
        virtual StringHandler ActionDcsDispatch(const VTID id, const VTParameters parameters) = 0;
        virtual bool ActionOscDispatch(const size_t parameter, const std::u32string_view string) = 0;
        virtual bool ActionSs3Dispatch(const char32_t ch, const VTParameters parameters) = 0;

    protected:
        IStateMachineEngine() = default;
    };

    inline IStateMachineEngine::~IStateMachineEngine() = default;
}
