// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <QString>
#include <QColor>
#include <QJsonObject>
#include <QMap>
#include <optional>

namespace LTerm {

// ── Profile-level enums ───────────────────────────────────────────────────────

enum class CloseOnExit   { Automatic, Graceful, Always, Never };
enum class ScrollbarState{ Visible, Hidden, Always };
enum class BellStyle     { None, Audible, Visual, All };
enum class CursorShape   { Bar, Vintage, Underscore, FilledBox, EmptyBox };
enum class FontWeight    { Thin, ExtraLight, Light, SemiLight, Normal,
                           Medium, SemiBold, Bold, ExtraBold, Black };
enum class IntenseTextStyle { Bright, Bold, All };
enum class BgStretchMode { None, Fill, Uniform, UniformToFill };
enum class BgAlignment   {
    TopLeft,    TopCenter,    TopRight,
    MiddleLeft, MiddleCenter, MiddleRight,
    BottomLeft, BottomCenter, BottomRight
};

/**
 * Profile — one terminal profile (shell, font, color scheme, and all
 * per-profile settings mirroring Windows Terminal's Profile model).
 */
struct Profile
{
    // ── Identity ──────────────────────────────────────────────────────────────
    QString name           { QStringLiteral("Default") };
    QString guid           {};  // UUID string, auto-generated on first save

    // ── General ───────────────────────────────────────────────────────────────
    QString shell          {};  // empty → use $SHELL
    QString workingDirectory {};// empty → user home dir
    QString tabTitle       {};  // empty → use process title
    QString tabColor       {};  // hex "#RRGGBB" or empty (use default tab color)
    bool    hidden                  { false };
    bool    suppressApplicationTitle{ false };
    CloseOnExit closeOnExit         { CloseOnExit::Automatic };

    // ── Font ──────────────────────────────────────────────────────────────────
    QString    fontFamily        { QStringLiteral("Monospace") };
    int        fontSize          { 11 };
    FontWeight fontWeight        { FontWeight::Normal };
    bool       enableBuiltinGlyphs  { true };
    bool       enableColorGlyphs    { true };

    // ── Appearance ────────────────────────────────────────────────────────────
    QString    colorScheme       { QStringLiteral("Default") };
    CursorShape cursorShape      { CursorShape::Bar };
    int         cursorHeight     { 25 };  // % cell height for Underscore shape

    // Nullable color overrides — nullopt means "use scheme default".
    std::optional<QColor> cursorColor;
    std::optional<QColor> foregroundColorOverride;
    std::optional<QColor> backgroundColorOverride;
    std::optional<QColor> selectionBackground;

    // These settings are persisted but rendering is not yet implemented.
    double          opacity                     { 1.0 };
    bool            retroTerminalEffect         { false };
    IntenseTextStyle intenseTextStyle           { IntenseTextStyle::Bright };
    bool            adjustIndistinguishableColors{ true };

    // ── Background image ──────────────────────────────────────────────────────
    QString       backgroundImagePath          {};  // empty → no background image
    double        backgroundImageOpacity       { 0.5 };
    BgStretchMode backgroundImageStretchMode   { BgStretchMode::UniformToFill };
    BgAlignment   backgroundImageAlignment     { BgAlignment::MiddleCenter };

    // ── Advanced ──────────────────────────────────────────────────────────────
    int            historySize     { 9001 };
    ScrollbarState scrollbarState  { ScrollbarState::Visible };
    int            padding         { 8 };
    bool           snapOnInput     { true };
    bool           altGrAliasing   { true };
    BellStyle      bellStyle       { BellStyle::Audible };
    QMap<QString, QString> environmentVariables;

    static Profile fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
    static Profile Default();
};

} // namespace LTerm
