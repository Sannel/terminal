// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "ColorScheme.hpp"
#include "GlobalSettings.hpp"
#include "Profile.hpp"

#include <QObject>
#include <QMap>
#include <QString>

namespace LTerm {

/**
 * LTermSettings — application-wide settings (global + profiles + color schemes).
 *
 * Settings are stored as JSON at:
 *   $XDG_CONFIG_HOME/lterm/settings.json
 *   (~/.config/lterm/settings.json)
 *
 * Call load() on startup, save() after any change.
 */
class LTermSettings : public QObject
{
    Q_OBJECT
public:
    static LTermSettings& instance();

    /** Load settings from disk. Creates defaults if the file does not exist. */
    void load();

    /** Persist settings to disk. */
    void save() const;

    // ── Global settings ───────────────────────────────────────────────────────
    const GlobalSettings& global() const noexcept { return _global; }
    void setGlobal(const GlobalSettings& g);

    // ── Profiles ──────────────────────────────────────────────────────────────
    const QMap<QString, Profile>& profiles() const noexcept { return _profiles; }
    Profile profile(const QString& name) const;
    void setProfile(const Profile& p);
    void removeProfile(const QString& name);

    QString defaultProfileName() const noexcept { return _global.defaultProfileName; }
    void setDefaultProfileName(const QString& name);

    Profile defaultProfile() const { return profile(_global.defaultProfileName); }

    // ── Color Schemes ─────────────────────────────────────────────────────────
    const QMap<QString, ColorScheme>& colorSchemes() const noexcept { return _colorSchemes; }
    ColorScheme colorScheme(const QString& name) const;
    void setColorScheme(const ColorScheme& cs);

signals:
    void settingsChanged();

private:
    LTermSettings();

    QString _settingsPath() const;
    void _ensureDefaults();

    GlobalSettings             _global;
    QMap<QString, Profile>     _profiles;
    QMap<QString, ColorScheme> _colorSchemes;
};

} // namespace LTerm
