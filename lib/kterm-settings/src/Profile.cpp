// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "Profile.hpp"

#include <QJsonObject>

namespace KTerm {

Profile Profile::fromJson(const QJsonObject& obj)
{
    Profile p;
    p.name             = obj[QStringLiteral("name")].toString(p.name);
    p.shell            = obj[QStringLiteral("shell")].toString(p.shell);
    p.workingDirectory = obj[QStringLiteral("workingDirectory")].toString(p.workingDirectory);
    p.fontFamily       = obj[QStringLiteral("font")].toString(p.fontFamily);
    p.fontSize         = obj[QStringLiteral("fontSize")].toInt(p.fontSize);
    p.colorScheme      = obj[QStringLiteral("colorScheme")].toString(p.colorScheme);
    return p;
}

QJsonObject Profile::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")]             = name;
    obj[QStringLiteral("shell")]            = shell;
    obj[QStringLiteral("workingDirectory")] = workingDirectory;
    obj[QStringLiteral("font")]             = fontFamily;
    obj[QStringLiteral("fontSize")]         = fontSize;
    obj[QStringLiteral("colorScheme")]      = colorScheme;
    return obj;
}

Profile Profile::Default()
{
    return Profile{};
}

} // namespace KTerm
