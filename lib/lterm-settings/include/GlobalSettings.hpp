// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <QString>
#include <QJsonObject>

namespace LTerm {

enum class LaunchMode    { Default, Maximized, Fullscreen, Focus };
enum class TabWidthMode  { Equal, Compact, TitleLength };
enum class NewTabPosition{ AfterLastTab, AfterCurrentTab };
enum class ConfirmOnClose{ Never, Automatic, Always };
enum class AppTheme      { System, Dark, Light };

/**
 * GlobalSettings — application-wide settings that are not profile-specific.
 *
 * Mirrors Windows Terminal's GlobalAppSettings model, excluding Windows-only
 * features (Acrylic, Mica, elevation, WSL, pixel shaders, system tray, etc.).
 */
struct GlobalSettings
{
    // ── Startup ───────────────────────────────────────────────────────────────
    QString      defaultProfileName { QStringLiteral("Default") };
    int          initialRows        { 30 };
    int          initialCols        { 120 };
    LaunchMode   launchMode         { LaunchMode::Default };
    bool         centerOnLaunch     { false };
    bool         alwaysOnTop        { false };
    bool         alwaysShowTabs     { false };
    TabWidthMode tabWidthMode       { TabWidthMode::Equal };
    NewTabPosition newTabPosition   { NewTabPosition::AfterLastTab };

    // ── Interaction (plumbing TODO — persisted but not yet active) ────────────
    bool         copyOnSelect            { false };
    bool         trimPaste               { true };
    bool         warnAboutLargePaste     { true };
    bool         warnAboutMultiLinePaste { true };
    QString      wordDelimiters          { QStringLiteral(" \t[]{}|;&<>()\"'") };
    bool         trimBlockSelection      { true };
    bool         focusFollowMouse        { false };
    bool         detectURLs              { true };
    bool         snapToGridOnResize      { true };

    // ── Appearance ────────────────────────────────────────────────────────────
    AppTheme       theme          { AppTheme::System };
    ConfirmOnClose confirmOnClose { ConfirmOnClose::Automatic };

    static GlobalSettings fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;
    static GlobalSettings Default();
};

} // namespace LTerm
