// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include <QString>
#include <QJsonObject>

namespace LTerm {

/**
 * Profile — one terminal profile (shell, font, color scheme).
 */
struct Profile
{
    QString name           { QStringLiteral("Default") };
    QString shell          {};           // empty → use $SHELL
    QString workingDirectory {};         // empty → user home
    QString fontFamily     { QStringLiteral("Monospace") };
    int     fontSize       { 11 };
    QString colorScheme    { QStringLiteral("Default") };

    static Profile fromJson(const QJsonObject& obj);
    QJsonObject toJson() const;

    /** Default profile singleton. */
    static Profile Default();
};

} // namespace LTerm
