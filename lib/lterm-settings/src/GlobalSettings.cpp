// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "GlobalSettings.hpp"

#include <QJsonObject>

namespace LTerm {

// ── Enum helpers ──────────────────────────────────────────────────────────────

static LaunchMode parseLaunchMode(const QString& s)
{
    if (s == QStringLiteral("maximized"))  return LaunchMode::Maximized;
    if (s == QStringLiteral("fullscreen")) return LaunchMode::Fullscreen;
    if (s == QStringLiteral("focus"))      return LaunchMode::Focus;
    return LaunchMode::Default;
}

static QString launchModeStr(LaunchMode m)
{
    switch (m) {
    case LaunchMode::Maximized:  return QStringLiteral("maximized");
    case LaunchMode::Fullscreen: return QStringLiteral("fullscreen");
    case LaunchMode::Focus:      return QStringLiteral("focus");
    default:                     return QStringLiteral("default");
    }
}

static TabWidthMode parseTabWidthMode(const QString& s)
{
    if (s == QStringLiteral("compact"))      return TabWidthMode::Compact;
    if (s == QStringLiteral("titleLength"))  return TabWidthMode::TitleLength;
    return TabWidthMode::Equal;
}

static QString tabWidthModeStr(TabWidthMode m)
{
    switch (m) {
    case TabWidthMode::Compact:      return QStringLiteral("compact");
    case TabWidthMode::TitleLength:  return QStringLiteral("titleLength");
    default:                         return QStringLiteral("equal");
    }
}

static NewTabPosition parseNewTabPosition(const QString& s)
{
    if (s == QStringLiteral("afterCurrentTab")) return NewTabPosition::AfterCurrentTab;
    return NewTabPosition::AfterLastTab;
}

static QString newTabPositionStr(NewTabPosition p)
{
    switch (p) {
    case NewTabPosition::AfterCurrentTab: return QStringLiteral("afterCurrentTab");
    default:                              return QStringLiteral("afterLastTab");
    }
}

static ConfirmOnClose parseConfirmOnClose(const QString& s)
{
    if (s == QStringLiteral("never"))  return ConfirmOnClose::Never;
    if (s == QStringLiteral("always")) return ConfirmOnClose::Always;
    return ConfirmOnClose::Automatic;
}

static QString confirmOnCloseStr(ConfirmOnClose c)
{
    switch (c) {
    case ConfirmOnClose::Never:  return QStringLiteral("never");
    case ConfirmOnClose::Always: return QStringLiteral("always");
    default:                     return QStringLiteral("automatic");
    }
}

static AppTheme parseTheme(const QString& s)
{
    if (s == QStringLiteral("dark"))  return AppTheme::Dark;
    if (s == QStringLiteral("light")) return AppTheme::Light;
    return AppTheme::System;
}

static QString themeStr(AppTheme t)
{
    switch (t) {
    case AppTheme::Dark:  return QStringLiteral("dark");
    case AppTheme::Light: return QStringLiteral("light");
    default:              return QStringLiteral("system");
    }
}

// ── fromJson ──────────────────────────────────────────────────────────────────

GlobalSettings GlobalSettings::fromJson(const QJsonObject& obj)
{
    GlobalSettings g;
    g.defaultProfileName = obj[QStringLiteral("defaultProfile")].toString(g.defaultProfileName);
    g.initialRows        = obj[QStringLiteral("initialRows")].toInt(g.initialRows);
    g.initialCols        = obj[QStringLiteral("initialCols")].toInt(g.initialCols);
    g.launchMode         = parseLaunchMode(obj[QStringLiteral("launchMode")].toString());
    g.centerOnLaunch     = obj[QStringLiteral("centerOnLaunch")].toBool(g.centerOnLaunch);
    g.alwaysOnTop        = obj[QStringLiteral("alwaysOnTop")].toBool(g.alwaysOnTop);
    g.alwaysShowTabs     = obj[QStringLiteral("alwaysShowTabs")].toBool(g.alwaysShowTabs);
    g.tabWidthMode       = parseTabWidthMode(obj[QStringLiteral("tabWidthMode")].toString());
    g.newTabPosition     = parseNewTabPosition(obj[QStringLiteral("newTabPosition")].toString());

    g.copyOnSelect            = obj[QStringLiteral("copyOnSelect")].toBool(g.copyOnSelect);
    g.trimPaste               = obj[QStringLiteral("trimPaste")].toBool(g.trimPaste);
    g.warnAboutLargePaste     = obj[QStringLiteral("warnAboutLargePaste")].toBool(g.warnAboutLargePaste);
    g.warnAboutMultiLinePaste = obj[QStringLiteral("warnAboutMultiLinePaste")].toBool(g.warnAboutMultiLinePaste);
    g.wordDelimiters          = obj[QStringLiteral("wordDelimiters")].toString(g.wordDelimiters);
    g.trimBlockSelection      = obj[QStringLiteral("trimBlockSelection")].toBool(g.trimBlockSelection);
    g.focusFollowMouse        = obj[QStringLiteral("focusFollowMouse")].toBool(g.focusFollowMouse);
    g.detectURLs              = obj[QStringLiteral("detectURLs")].toBool(g.detectURLs);
    g.snapToGridOnResize      = obj[QStringLiteral("snapToGridOnResize")].toBool(g.snapToGridOnResize);

    g.theme          = parseTheme(obj[QStringLiteral("theme")].toString());
    g.confirmOnClose = parseConfirmOnClose(obj[QStringLiteral("confirmOnClose")].toString());

    return g;
}

// ── toJson ────────────────────────────────────────────────────────────────────

QJsonObject GlobalSettings::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("defaultProfile")]      = defaultProfileName;
    obj[QStringLiteral("initialRows")]         = initialRows;
    obj[QStringLiteral("initialCols")]         = initialCols;
    obj[QStringLiteral("launchMode")]          = launchModeStr(launchMode);
    obj[QStringLiteral("centerOnLaunch")]      = centerOnLaunch;
    obj[QStringLiteral("alwaysOnTop")]         = alwaysOnTop;
    obj[QStringLiteral("alwaysShowTabs")]      = alwaysShowTabs;
    obj[QStringLiteral("tabWidthMode")]        = tabWidthModeStr(tabWidthMode);
    obj[QStringLiteral("newTabPosition")]      = newTabPositionStr(newTabPosition);

    obj[QStringLiteral("copyOnSelect")]            = copyOnSelect;
    obj[QStringLiteral("trimPaste")]               = trimPaste;
    obj[QStringLiteral("warnAboutLargePaste")]     = warnAboutLargePaste;
    obj[QStringLiteral("warnAboutMultiLinePaste")] = warnAboutMultiLinePaste;
    obj[QStringLiteral("wordDelimiters")]          = wordDelimiters;
    obj[QStringLiteral("trimBlockSelection")]      = trimBlockSelection;
    obj[QStringLiteral("focusFollowMouse")]        = focusFollowMouse;
    obj[QStringLiteral("detectURLs")]              = detectURLs;
    obj[QStringLiteral("snapToGridOnResize")]      = snapToGridOnResize;

    obj[QStringLiteral("theme")]          = themeStr(theme);
    obj[QStringLiteral("confirmOnClose")] = confirmOnCloseStr(confirmOnClose);

    return obj;
}

GlobalSettings GlobalSettings::Default()
{
    return GlobalSettings{};
}

} // namespace LTerm
