// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
// KTerm port: wchar_t → char32_t, BYTE → uint8_t, removed Win32 types.

#pragma once

#include <bit>
#include <climits>
#include <cstdint>
#include <functional>
#include <span>
#include <utility>

#include "til/at.h"

namespace Microsoft::Console::VirtualTerminal
{
    using VTInt = int32_t;

    union VTID
    {
    public:
        VTID() = default;

        template<size_t Length>
        constexpr VTID(const char (&s)[Length]) :
            _value{ _FromString(s) }
        {
        }

        constexpr VTID(const uint64_t value) :
            _value{ value & 0x00FFFFFFFFFFFFFF }
        {
        }

        constexpr operator uint64_t() const { return _value; }

        constexpr const char* ToString() const { return &_string[0]; }

        constexpr char operator[](const size_t offset) const
        {
            return SubSequence(offset)._value & 0xFF;
        }

        constexpr VTID SubSequence(const size_t offset) const
        {
            return _value >> (CHAR_BIT * offset);
        }

    private:
        template<size_t Length>
        static constexpr uint64_t _FromString(const char (&s)[Length])
        {
            static_assert(Length <= sizeof(_value));
            uint64_t value = 0;
            for (auto i = Length - 1; i-- > 0;)
            {
                value = (value << CHAR_BIT) + s[i];
            }
            return value;
        }

        static_assert(std::endian::native == std::endian::little);
        uint64_t _value = 0;
        char _string[sizeof(_value)];
    };

    class VTIDBuilder
    {
    public:
        void Clear() noexcept
        {
            _idAccumulator = 0;
            _idShift = 0;
        }

        void AddIntermediate(const char32_t ch) noexcept
        {
            if (_idShift + CHAR_BIT * 2 >= sizeof(_idAccumulator) * CHAR_BIT)
            {
                _idAccumulator = 0;
            }
            else
            {
                _idAccumulator += (static_cast<uint64_t>(ch) << _idShift);
                _idShift += CHAR_BIT;
            }
        }

        VTID Finalize(const char32_t finalChar) noexcept
        {
            return _idAccumulator + (static_cast<uint64_t>(finalChar) << _idShift);
        }

    private:
        uint64_t _idAccumulator = 0;
        size_t _idShift = 0;
    };

    class VTParameter
    {
    public:
        constexpr VTParameter() noexcept : _value{ -1 } {}
        constexpr VTParameter(const VTInt rhs) noexcept : _value{ rhs } {}

        constexpr bool has_value() const noexcept { return _value >= 0; }
        constexpr VTInt value() const noexcept { return _value; }
        constexpr VTInt value_or(VTInt defaultValue) const noexcept
        {
            return _value < 0 ? defaultValue : _value;
        }

        template<typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
        constexpr operator T() const noexcept { return static_cast<T>(value_or(0)); }

        constexpr operator VTInt() const noexcept { return _value <= 0 ? 1 : _value; }

    private:
        VTInt _value;
    };

    class VTSubParameters
    {
    public:
        constexpr VTSubParameters() noexcept = default;
        constexpr VTSubParameters(const std::span<const VTParameter> subParams) noexcept :
            _subParams{ subParams } {}

        constexpr VTParameter at(const size_t index) const noexcept
        {
            return index < _subParams.size() ? til::at(_subParams, index) : defaultParameter;
        }

        VTSubParameters subspan(const size_t offset, const size_t count) const noexcept
        {
            return { _subParams.subspan(offset, count) };
        }

        bool empty() const noexcept { return _subParams.empty(); }
        size_t size() const noexcept { return _subParams.size(); }

        constexpr operator std::span<const VTParameter>() const noexcept { return _subParams; }

    private:
        static constexpr VTParameter defaultParameter{};
        std::span<const VTParameter> _subParams;
    };

    class VTParameters
    {
    public:
        constexpr VTParameters() noexcept = default;

        constexpr VTParameters(const VTParameter* paramsPtr, const size_t paramsCount) noexcept :
            _params{ paramsPtr, paramsCount } {}

        constexpr VTParameters(const std::span<const VTParameter> params,
                               const std::span<const VTParameter> subParams,
                               const std::span<const std::pair<uint8_t, uint8_t>> subParamRanges) noexcept :
            _params{ params },
            _subParams{ subParams },
            _subParamRanges{ subParamRanges }
        {
        }

        constexpr VTParameter at(const size_t index) const noexcept
        {
            return index < _params.size() ? til::at(_params, index) : defaultParameter;
        }

        constexpr bool empty() const noexcept { return _params.empty(); }

        constexpr size_t size() const noexcept
        {
            return std::max<size_t>(_params.size(), 1);
        }

        VTParameters subspan(const size_t offset) const noexcept
        {
            const auto newParamsSpan = _params.subspan(std::min(offset, _params.size()));
            const auto newSubParamRangesSpan = _subParamRanges.subspan(std::min(offset, _subParamRanges.size()));
            return { newParamsSpan, _subParams, newSubParamRangesSpan };
        }

        VTSubParameters subParamsFor(const size_t index) const noexcept
        {
            if (index < _subParamRanges.size())
            {
                const auto& range = til::at(_subParamRanges, index);
                return _subParams.subspan(range.first, range.second - range.first);
            }
            return {};
        }

        bool hasSubParams() const noexcept { return !_subParams.empty(); }

        bool hasSubParamsFor(const size_t index) const noexcept
        {
            if (index < _subParamRanges.size())
            {
                const auto& range = til::at(_subParamRanges, index);
                return range.second > range.first;
            }
            return false;
        }

        template<typename T>
        void for_each(const T&& predicate) const
        {
            auto params = _params;
            if (params.empty())
            {
                params = defaultParameters;
            }
            for (const auto& v : params)
            {
                predicate(v);
            }
        }

    private:
        static constexpr VTParameter defaultParameter{};
        static constexpr std::span<const VTParameter> defaultParameters{ &defaultParameter, 1 };

        std::span<const VTParameter> _params;
        VTSubParameters _subParams;
        std::span<const std::pair<uint8_t, uint8_t>> _subParamRanges;
    };
}
