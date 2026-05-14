// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "LTermSettings.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace LTerm {

LTermSettings& LTermSettings::instance()
{
    static LTermSettings inst;
    return inst;
}

LTermSettings::LTermSettings() : QObject(nullptr)
{
    _ensureDefaults();
}

// ── File path ─────────────────────────────────────────────────────────────────

QString LTermSettings::_settingsPath() const
{
    const QString configDir = QStandardPaths::writableLocation(
        QStandardPaths::AppConfigLocation);
    return configDir + QStringLiteral("/settings.json");
}

// ── Load / Save ───────────────────────────────────────────────────────────────

void LTermSettings::load()
{
    const QString path = _settingsPath();

    // One-time migration: copy old kterm config if lterm config missing.
    if (!QFile::exists(path)) {
        const QString oldDir = QStandardPaths::writableLocation(
            QStandardPaths::GenericConfigLocation) + QStringLiteral("/kterm");
        const QString oldPath = oldDir + QStringLiteral("/settings.json");
        if (QFile::exists(oldPath)) {
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile::copy(oldPath, path);
        }
    }

    QFile file(path);

    if (!file.exists()) {
        // First run: write defaults.
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

    // Load global settings (new format).
    if (root.contains(QStringLiteral("global"))) {
        _global = GlobalSettings::fromJson(root[QStringLiteral("global")].toObject());
    } else {
        // Migrate old flat format: "defaultProfile" was a top-level key.
        _global.defaultProfileName = root[QStringLiteral("defaultProfile")]
                                         .toString(_global.defaultProfileName);
    }

    // Load profiles.
    const QJsonObject profiles = root[QStringLiteral("profiles")].toObject();
    for (const QString& key : profiles.keys()) {
        _profiles[key] = Profile::fromJson(profiles[key].toObject());
    }

    // Load color schemes.
    const QJsonObject schemes = root[QStringLiteral("colorSchemes")].toObject();
    for (const QString& key : schemes.keys()) {
        _colorSchemes[key] = ColorScheme::fromJson(schemes[key].toObject());
    }

    // Guarantee built-in schemes are always present.
    _ensureDefaults();
}

void LTermSettings::save() const
{
    const QString path = _settingsPath();

    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonObject root;
    root[QStringLiteral("global")] = _global.toJson();

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

    // Atomic write.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

// ── Global ────────────────────────────────────────────────────────────────────

void LTermSettings::setGlobal(const GlobalSettings& g)
{
    _global = g;
    emit settingsChanged();
}

// ── Profiles ──────────────────────────────────────────────────────────────────

Profile LTermSettings::profile(const QString& name) const
{
    return _profiles.value(name, Profile::Default());
}

void LTermSettings::setProfile(const Profile& p)
{
    _profiles[p.name] = p;
    emit settingsChanged();
}

void LTermSettings::removeProfile(const QString& name)
{
    _profiles.remove(name);
    if (_global.defaultProfileName == name && !_profiles.isEmpty()) {
        _global.defaultProfileName = _profiles.firstKey();
    }
    emit settingsChanged();
}

void LTermSettings::setDefaultProfileName(const QString& name)
{
    _global.defaultProfileName = name;
    emit settingsChanged();
}

// ── Color Schemes ─────────────────────────────────────────────────────────────

ColorScheme LTermSettings::colorScheme(const QString& name) const
{
    return _colorSchemes.value(name, ColorScheme::Default());
}

void LTermSettings::setColorScheme(const ColorScheme& cs)
{
    _colorSchemes[cs.name] = cs;
    emit settingsChanged();
}

void LTermSettings::removeColorScheme(const QString& name)
{
    _colorSchemes.remove(name);
    emit settingsChanged();
}

// ── Defaults ──────────────────────────────────────────────────────────────────

void LTermSettings::_ensureDefaults()
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

} // namespace LTerm
