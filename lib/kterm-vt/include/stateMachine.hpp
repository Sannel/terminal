// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// KTerm port: wchar_t → char32_t, BYTE → uint8_t, removed ETW tracing, Win32 types.

#pragma once

#include "IStateMachineEngine.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "til/enumset.h"
#include "til/small_vector.h"

namespace Microsoft::Console::VirtualTerminal
{
    // Max parameter value per DEC STD 070 / XTerm / VTE.
    constexpr VTInt MAX_PARAMETER_VALUE = 65535;
    constexpr size_t MAX_PARAMETER_COUNT = 32;
    constexpr size_t MAX_SUBPARAMETER_COUNT = 6;
    static_assert(MAX_PARAMETER_COUNT * MAX_SUBPARAMETER_COUNT <= 256);

    enum class InjectionType : size_t
    {
        RIS,
        DECSET_FOCUS,
        W32IM,
        Count,
    };

    struct Injection
    {
        InjectionType type;
        size_t offset;
    };

    class StateMachine final
    {
    public:
        template<typename T>
        StateMachine(std::unique_ptr<T> engine) noexcept :
            StateMachine(std::move(engine), std::is_same_v<T, class InputStateMachineEngine>)
        {
        }
        StateMachine(std::unique_ptr<IStateMachineEngine> engine, const bool isEngineForInput) noexcept;

        enum class Mode : size_t
        {
            AcceptC1,
            Ansi,
        };

        void SetParserMode(const Mode mode, const bool enabled) noexcept;
        bool GetParserMode(const Mode mode) const noexcept;

        void ProcessCharacter(const char32_t ch);
        void ProcessString(const std::u32string_view string);
        bool IsProcessingLastCharacter() const noexcept;

        void InjectSequence(InjectionType type);
        const til::small_vector<Injection, 8>& GetInjections() const noexcept;

        void OnCsiComplete(const std::function<void()> callback);
        void ResetState() noexcept;
        bool FlushToTerminal();

        const IStateMachineEngine& Engine() const noexcept;
        IStateMachineEngine& Engine() noexcept;

    private:
        void _ActionExecute(const char32_t ch);
        void _ActionExecuteFromEscape(const char32_t ch);
        void _ActionPrint(const char32_t ch);
        void _ActionPrintString(const std::u32string_view string);
        void _ActionEscDispatch(const char32_t ch);
        void _ActionVt52EscDispatch(const char32_t ch);
        void _ActionCollect(const char32_t ch) noexcept;
        void _ActionParam(const char32_t ch);
        void _ActionSubParam(const char32_t ch);
        void _ActionCsiDispatch(const char32_t ch);
        void _ActionOscParam(const char32_t ch) noexcept;
        void _ActionOscPut(const char32_t ch);
        void _ActionOscDispatch();
        void _ActionSs3Dispatch(const char32_t ch);
        void _ActionDcsDispatch(const char32_t ch);

        void _ActionClear() noexcept;
        void _ActionIgnore() noexcept;
        void _ActionInterrupt();

        void _EnterGround() noexcept;
        void _EnterEscape() noexcept;
        void _EnterEscapeIntermediate() noexcept;
        void _EnterCsiEntry() noexcept;
        void _EnterCsiParam() noexcept;
        void _EnterCsiSubParam() noexcept;
        void _EnterCsiIgnore() noexcept;
        void _EnterCsiIntermediate() noexcept;
        void _EnterOscParam() noexcept;
        void _EnterOscString() noexcept;
        void _EnterOscTermination() noexcept;
        void _EnterSs3Entry() noexcept;
        void _EnterSs3Param() noexcept;
        void _EnterVt52Param() noexcept;
        void _EnterDcsEntry() noexcept;
        void _EnterDcsParam() noexcept;
        void _EnterDcsIgnore() noexcept;
        void _EnterDcsIntermediate() noexcept;
        void _EnterDcsPassThrough() noexcept;
        void _EnterSosPmApcString() noexcept;

        void _EventGround(const char32_t ch);
        void _EventEscape(const char32_t ch);
        void _EventEscapeIntermediate(const char32_t ch);
        void _EventCsiEntry(const char32_t ch);
        void _EventCsiIntermediate(const char32_t ch);
        void _EventCsiIgnore(const char32_t ch);
        void _EventCsiParam(const char32_t ch);
        void _EventCsiSubParam(const char32_t ch);
        void _EventOscParam(const char32_t ch);
        void _EventOscString(const char32_t ch);
        void _EventOscTermination(const char32_t ch);
        void _EventSs3Entry(const char32_t ch);
        void _EventSs3Param(const char32_t ch);
        void _EventVt52Param(const char32_t ch);
        void _EventDcsEntry(const char32_t ch);
        void _EventDcsIgnore() noexcept;
        void _EventDcsIntermediate(const char32_t ch);
        void _EventDcsParam(const char32_t ch);
        void _EventDcsPassThrough(const char32_t ch);
        void _EventSosPmApcString(const char32_t ch) noexcept;

        void _AccumulateTo(const char32_t ch, VTInt& value) noexcept;

        template<typename TLambda>
        bool _SafeExecute(TLambda&& lambda);

        void _ExecuteCsiCompleteCallback();

        enum class VTStates
        {
            Ground,
            Escape,
            EscapeIntermediate,
            CsiEntry,
            CsiIntermediate,
            CsiIgnore,
            CsiParam,
            CsiSubParam,
            OscParam,
            OscString,
            OscTermination,
            Ss3Entry,
            Ss3Param,
            Vt52Param,
            DcsEntry,
            DcsIgnore,
            DcsIntermediate,
            DcsParam,
            DcsPassThrough,
            SosPmApcString
        };

        std::unique_ptr<IStateMachineEngine> _engine;
        const bool _isEngineForInput;

        VTStates _state;

        til::enumset<Mode> _parserMode{ Mode::Ansi };

        std::u32string_view _currentString;
        size_t _runOffset;
        size_t _runSize;

        std::u32string_view _CurrentRun() const
        {
            return _currentString.substr(_runOffset, _runSize);
        }

        VTIDBuilder _identifier;
        std::vector<VTParameter> _parameters;
        bool _parameterLimitOverflowed;
        std::vector<VTParameter> _subParameters;
        std::vector<std::pair<uint8_t, uint8_t>> _subParameterRanges;
        bool _subParameterLimitOverflowed;
        uint8_t _subParameterCounter;

        std::u32string _oscString;
        VTInt _oscParameter;

        IStateMachineEngine::StringHandler _dcsStringHandler;

        std::optional<std::u32string> _cachedSequence;
        til::small_vector<Injection, 8> _injections;

        bool _processingLastCharacter;

        std::function<void()> _onCsiCompleteCallback;
    };
}
