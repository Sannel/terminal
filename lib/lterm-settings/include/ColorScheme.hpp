// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>
#include <array>

namespace LTerm {

/**
 * ColorScheme — 16 ANSI colors plus fg/bg/cursor defaults.
 */
struct ColorScheme
{
    QString name;

    QColor background    { 0x1E, 0x1E, 0x1E };
    QColor foreground    { 0xCC, 0xCC, 0xCC };
    QColor cursor        { 0xFF, 0xFF, 0xFF };
    QColor selectionBg   { 0x26, 0x4F, 0x78 };

    // ANSI 0-15
    std::array<QColor, 16> ansiColors {{
        QColor(0x00, 0x00, 0x00), // 0 Black
        QColor(0x80, 0x00, 0x00), // 1 Red
        QColor(0x00, 0x80, 0x00), // 2 Green
        QColor(0x80, 0x80, 0x00), // 3 Yellow
        QColor(0x00, 0x00, 0x80), // 4 Blue
        QColor(0x80, 0x00, 0x80), // 5 Magenta
        QColor(0x00, 0x80, 0x80), // 6 Cyan
        QColor(0xC0, 0xC0, 0xC0), // 7 White
        QColor(0x80, 0x80, 0x80), // 8  Bright Black
        QColor(0xFF, 0x00, 0x00), // 9  Bright Red
        QColor(0x00, 0xFF, 0x00), // 10 Bright Green
        QColor(0xFF, 0xFF, 0x00), // 11 Bright Yellow
        QColor(0x00, 0x00, 0xFF), // 12 Bright Blue
        QColor(0xFF, 0x00, 0xFF), // 13 Bright Magenta
        QColor(0x00, 0xFF, 0xFF), // 14 Bright Cyan
        QColor(0xFF, 0xFF, 0xFF), // 15 Bright White
    }};

    static ColorScheme fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;

    /** Built-in "One Dark" color scheme. */
    static ColorScheme OneDark();
    /** Built-in "Solarized Dark" color scheme. */
    static ColorScheme SolarizedDark();
    /** Built-in classic dark scheme (default if no config). */
    static ColorScheme Default();
};

} // namespace LTerm
