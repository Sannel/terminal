// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <cstdint>

namespace KTerm
{
    // Color representation supporting Default, 16-color index,
    // 256-color index, and 24-bit RGB.
    enum class ColorType : uint8_t
    {
        Default = 0,
        Index16,
        Index256,
        RGB,
    };

    struct TextColor
    {
        ColorType type = ColorType::Default;
        uint8_t r = 0; // also used as color index for Index16/Index256
        uint8_t g = 0;
        uint8_t b = 0;

        static TextColor Default() noexcept { return {}; }
        static TextColor FromIndex(uint8_t idx) noexcept { return { ColorType::Index16, idx, 0, 0 }; }
        static TextColor From256(uint8_t idx) noexcept { return { ColorType::Index256, idx, 0, 0 }; }
        static TextColor FromRGB(uint8_t r, uint8_t g, uint8_t b) noexcept { return { ColorType::RGB, r, g, b }; }

        bool isDefault() const noexcept { return type == ColorType::Default; }
        uint8_t index() const noexcept { return r; }
    };

    struct TextAttribute
    {
        TextColor fg;
        TextColor bg;

        bool bold          : 1;
        bool faint         : 1;
        bool italic        : 1;
        bool underline     : 1;
        bool blink         : 1;
        bool inverse       : 1;
        bool invisible     : 1;
        bool strikethrough : 1;

        constexpr TextAttribute() noexcept :
            bold(false), faint(false), italic(false), underline(false),
            blink(false), inverse(false), invisible(false), strikethrough(false)
        {}
    };

    struct TextCell
    {
        char32_t ch   = U' ';
        TextAttribute attr;
        bool wide     = false; // double-width CJK character (occupies 2 columns)
    };
}
