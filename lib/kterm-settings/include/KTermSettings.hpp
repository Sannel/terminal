// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "ColorScheme.hpp"
#include "Profile.hpp"

#include <QObject>
#include <QMap>
#include <QString>

namespace KTerm {

/**
 * KTermSettings — application-wide settings (profiles + color schemes).
 *
 * Settings are stored as JSON at:
 *   $XDG_CONFIG_HOME/kterm/settings.json
 *   (~/.config/kterm/settings.json)
 *
 * Call load() on startup, save() after any change.
 */
class KTermSettings : public QObject
{
    Q_OBJECT
public:
    static KTermSettings& instance();

    /** Load settings from disk. Creates defaults if the file does not exist. */
    void load();

    /** Persist settings to disk. */
    void save() const;

    // ── Profiles ──────────────────────────────────────────────────────────
    const QMap<QString, Profile>& profiles() const noexcept { return _profiles; }
    Profile profile(const QString& name) const;
    void setProfile(const Profile& p);
    void removeProfile(const QString& name);

    QString defaultProfileName() const noexcept { return _defaultProfileName; }
    void setDefaultProfileName(const QString& name);

    Profile defaultProfile() const { return profile(_defaultProfileName); }

    // ── Color Schemes ─────────────────────────────────────────────────────
    const QMap<QString, ColorScheme>& colorSchemes() const noexcept { return _colorSchemes; }
    ColorScheme colorScheme(const QString& name) const;
    void setColorScheme(const ColorScheme& cs);

signals:
    void settingsChanged();

private:
    KTermSettings();

    QString _settingsPath() const;
    void _ensureDefaults();

    QString _defaultProfileName { QStringLiteral("Default") };
    QMap<QString, Profile>     _profiles;
    QMap<QString, ColorScheme> _colorSchemes;
};

} // namespace KTerm
