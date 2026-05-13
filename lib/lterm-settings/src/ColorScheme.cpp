// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "ColorScheme.hpp"

#include <QJsonArray>

namespace LTerm {

static QString colorHex(const QColor& c)
{
    return c.name(QColor::HexRgb);
}

static QColor parseColor(const QJsonValue& v, const QColor& fallback = Qt::black)
{
    if (v.isString()) {
        const QColor c(v.toString());
        return c.isValid() ? c : fallback;
    }
    return fallback;
}

// ── JSON round-trip ───────────────────────────────────────────────────────────

ColorScheme ColorScheme::fromJson(const QJsonObject& obj)
{
    ColorScheme cs;
    cs.name = obj[QStringLiteral("name")].toString(cs.name);
    cs.background  = parseColor(obj[QStringLiteral("background")],  cs.background);
    cs.foreground  = parseColor(obj[QStringLiteral("foreground")],  cs.foreground);
    cs.cursor      = parseColor(obj[QStringLiteral("cursor")],      cs.cursor);
    cs.selectionBg = parseColor(obj[QStringLiteral("selectionBackground")], cs.selectionBg);

    const QJsonArray colors = obj[QStringLiteral("ansiColors")].toArray();
    for (int i = 0; i < std::min(16, static_cast<int>(colors.size())); ++i) {
        const QColor c(colors[i].toString());
        if (c.isValid()) {
            cs.ansiColors[i] = c;
        }
    }
    return cs;
}

QJsonObject ColorScheme::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")]              = name;
    obj[QStringLiteral("background")]        = colorHex(background);
    obj[QStringLiteral("foreground")]        = colorHex(foreground);
    obj[QStringLiteral("cursor")]            = colorHex(cursor);
    obj[QStringLiteral("selectionBackground")] = colorHex(selectionBg);

    QJsonArray colors;
    for (const auto& c : ansiColors) {
        colors.append(colorHex(c));
    }
    obj[QStringLiteral("ansiColors")] = colors;
    return obj;
}

// ── Built-in schemes ──────────────────────────────────────────────────────────

ColorScheme ColorScheme::Default()
{
    ColorScheme cs;
    cs.name = QStringLiteral("Default");
    return cs;
}

ColorScheme ColorScheme::OneDark()
{
    ColorScheme cs;
    cs.name       = QStringLiteral("One Dark");
    cs.background = QColor(0x28, 0x2C, 0x34);
    cs.foreground = QColor(0xAB, 0xB2, 0xBF);
    cs.cursor     = QColor(0x52, 0x8B, 0xFF);
    cs.selectionBg= QColor(0x3E, 0x44, 0x51);
    cs.ansiColors = {{
        QColor(0x28, 0x2C, 0x34), QColor(0xE0, 0x6C, 0x75),
        QColor(0x98, 0xC3, 0x79), QColor(0xE5, 0xC0, 0x7B),
        QColor(0x61, 0xAF, 0xEF), QColor(0xC6, 0x78, 0xDD),
        QColor(0x56, 0xB6, 0xC2), QColor(0xAB, 0xB2, 0xBF),
        QColor(0x5C, 0x63, 0x70), QColor(0xE0, 0x6C, 0x75),
        QColor(0x98, 0xC3, 0x79), QColor(0xE5, 0xC0, 0x7B),
        QColor(0x61, 0xAF, 0xEF), QColor(0xC6, 0x78, 0xDD),
        QColor(0x56, 0xB6, 0xC2), QColor(0xFF, 0xFF, 0xFF),
    }};
    return cs;
}

ColorScheme ColorScheme::SolarizedDark()
{
    ColorScheme cs;
    cs.name       = QStringLiteral("Solarized Dark");
    cs.background = QColor(0x00, 0x2B, 0x36);
    cs.foreground = QColor(0x83, 0x94, 0x96);
    cs.cursor     = QColor(0x83, 0x94, 0x96);
    cs.selectionBg= QColor(0x07, 0x36, 0x42);
    cs.ansiColors = {{
        QColor(0x07, 0x36, 0x42), QColor(0xDC, 0x32, 0x2F),
        QColor(0x85, 0x99, 0x00), QColor(0xB5, 0x89, 0x00),
        QColor(0x26, 0x8B, 0xD2), QColor(0xD3, 0x36, 0x82),
        QColor(0x2A, 0xA1, 0x98), QColor(0xEE, 0xE8, 0xD5),
        QColor(0x00, 0x2B, 0x36), QColor(0xCB, 0x4B, 0x16),
        QColor(0x58, 0x6E, 0x75), QColor(0x65, 0x7B, 0x83),
        QColor(0x83, 0x94, 0x96), QColor(0x6C, 0x71, 0xC4),
        QColor(0x93, 0xA1, 0xA1), QColor(0xFD, 0xF6, 0xE3),
    }};
    return cs;
}

} // namespace LTerm
