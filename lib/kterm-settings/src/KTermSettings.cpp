// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "KTermSettings.hpp"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace KTerm {

KTermSettings& KTermSettings::instance()
{
    static KTermSettings inst;
    return inst;
}

KTermSettings::KTermSettings() : QObject(nullptr)
{
    _ensureDefaults();
}

// ── File path ─────────────────────────────────────────────────────────────────

QString KTermSettings::_settingsPath() const
{
    const QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    return configDir + QStringLiteral("/settings.json");
}

// ── Load / Save ───────────────────────────────────────────────────────────────

void KTermSettings::load()
{
    const QString path = _settingsPath();
    QFile file(path);

    if (!file.exists()) {
        // First run: write defaults, nothing more to do.
        save();
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject root = doc.object();

    _defaultProfileName = root[QStringLiteral("defaultProfile")]
                              .toString(_defaultProfileName);

    // Load profiles.
    const QJsonObject profiles = root[QStringLiteral("profiles")].toObject();
    for (const QString& key : profiles.keys()) {
        _profiles[key] = Profile::fromJson(profiles[key].toObject());
    }

    // Load color schemes.
    const QJsonObject schemes = root[QStringLiteral("colorSchemes")].toObject();
    for (const QString& key : schemes.keys()) {
        ColorScheme cs = ColorScheme::fromJson(schemes[key].toObject());
        _colorSchemes[key] = cs;
    }

    // Guarantee built-in schemes are always present (may be overridden by user).
    _ensureDefaults();
}

void KTermSettings::save() const
{
    const QString path = _settingsPath();

    // Ensure directory exists.
    QDir().mkpath(QFileInfo(path).absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QJsonObject root;
    root[QStringLiteral("defaultProfile")] = _defaultProfileName;

    QJsonObject profilesObj;
    for (auto it = _profiles.constBegin(); it != _profiles.constEnd(); ++it) {
        profilesObj[it.key()] = it.value().toJson();
    }
    root[QStringLiteral("profiles")] = profilesObj;

    QJsonObject schemesObj;
    for (auto it = _colorSchemes.constBegin(); it != _colorSchemes.constEnd(); ++it) {
        schemesObj[it.key()] = it.value().toJson();
    }
    root[QStringLiteral("colorSchemes")] = schemesObj;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// ── Profiles ──────────────────────────────────────────────────────────────────

Profile KTermSettings::profile(const QString& name) const
{
    return _profiles.value(name, Profile::Default());
}

void KTermSettings::setProfile(const Profile& p)
{
    _profiles[p.name] = p;
    emit settingsChanged();
}

void KTermSettings::removeProfile(const QString& name)
{
    _profiles.remove(name);
    if (_defaultProfileName == name && !_profiles.isEmpty()) {
        _defaultProfileName = _profiles.firstKey();
    }
    emit settingsChanged();
}

void KTermSettings::setDefaultProfileName(const QString& name)
{
    _defaultProfileName = name;
    emit settingsChanged();
}

// ── Color Schemes ─────────────────────────────────────────────────────────────

ColorScheme KTermSettings::colorScheme(const QString& name) const
{
    return _colorSchemes.value(name, ColorScheme::Default());
}

void KTermSettings::setColorScheme(const ColorScheme& cs)
{
    _colorSchemes[cs.name] = cs;
    emit settingsChanged();
}

// ── Defaults ──────────────────────────────────────────────────────────────────

void KTermSettings::_ensureDefaults()
{
    if (!_colorSchemes.contains(QStringLiteral("Default"))) {
        _colorSchemes[QStringLiteral("Default")] = ColorScheme::Default();
    }
    if (!_colorSchemes.contains(QStringLiteral("One Dark"))) {
        _colorSchemes[QStringLiteral("One Dark")] = ColorScheme::OneDark();
    }
    if (!_colorSchemes.contains(QStringLiteral("Solarized Dark"))) {
        _colorSchemes[QStringLiteral("Solarized Dark")] = ColorScheme::SolarizedDark();
    }

    if (_profiles.isEmpty()) {
        const Profile def = Profile::Default();
        _profiles[def.name] = def;
    }
}

} // namespace KTerm
