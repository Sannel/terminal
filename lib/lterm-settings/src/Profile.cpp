// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "Profile.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

namespace LTerm {

// ── Enum serialization helpers ────────────────────────────────────────────────

static CloseOnExit parseCloseOnExit(const QString& s)
{
    if (s == QStringLiteral("graceful")) return CloseOnExit::Graceful;
    if (s == QStringLiteral("always"))   return CloseOnExit::Always;
    if (s == QStringLiteral("never"))    return CloseOnExit::Never;
    return CloseOnExit::Automatic;
}
static QString closeOnExitStr(CloseOnExit v)
{
    switch (v) {
    case CloseOnExit::Graceful: return QStringLiteral("graceful");
    case CloseOnExit::Always:   return QStringLiteral("always");
    case CloseOnExit::Never:    return QStringLiteral("never");
    default:                    return QStringLiteral("automatic");
    }
}

static ScrollbarState parseScrollbarState(const QString& s)
{
    if (s == QStringLiteral("hidden")) return ScrollbarState::Hidden;
    if (s == QStringLiteral("always")) return ScrollbarState::Always;
    return ScrollbarState::Visible;
}
static QString scrollbarStateStr(ScrollbarState v)
{
    switch (v) {
    case ScrollbarState::Hidden: return QStringLiteral("hidden");
    case ScrollbarState::Always: return QStringLiteral("always");
    default:                     return QStringLiteral("visible");
    }
}

static BellStyle parseBellStyle(const QString& s)
{
    if (s == QStringLiteral("none"))   return BellStyle::None;
    if (s == QStringLiteral("visual")) return BellStyle::Visual;
    if (s == QStringLiteral("all"))    return BellStyle::All;
    return BellStyle::Audible;
}
static QString bellStyleStr(BellStyle v)
{
    switch (v) {
    case BellStyle::None:   return QStringLiteral("none");
    case BellStyle::Visual: return QStringLiteral("visual");
    case BellStyle::All:    return QStringLiteral("all");
    default:                return QStringLiteral("audible");
    }
}

static CursorShape parseCursorShape(const QString& s)
{
    if (s == QStringLiteral("vintage"))    return CursorShape::Vintage;
    if (s == QStringLiteral("underscore")) return CursorShape::Underscore;
    if (s == QStringLiteral("filledBox"))  return CursorShape::FilledBox;
    if (s == QStringLiteral("emptyBox"))   return CursorShape::EmptyBox;
    return CursorShape::Bar;
}
static QString cursorShapeStr(CursorShape v)
{
    switch (v) {
    case CursorShape::Vintage:    return QStringLiteral("vintage");
    case CursorShape::Underscore: return QStringLiteral("underscore");
    case CursorShape::FilledBox:  return QStringLiteral("filledBox");
    case CursorShape::EmptyBox:   return QStringLiteral("emptyBox");
    default:                      return QStringLiteral("bar");
    }
}

static FontWeight parseFontWeight(const QString& s)
{
    if (s == QStringLiteral("thin"))       return FontWeight::Thin;
    if (s == QStringLiteral("extraLight")) return FontWeight::ExtraLight;
    if (s == QStringLiteral("light"))      return FontWeight::Light;
    if (s == QStringLiteral("semiLight"))  return FontWeight::SemiLight;
    if (s == QStringLiteral("medium"))     return FontWeight::Medium;
    if (s == QStringLiteral("semiBold"))   return FontWeight::SemiBold;
    if (s == QStringLiteral("bold"))       return FontWeight::Bold;
    if (s == QStringLiteral("extraBold"))  return FontWeight::ExtraBold;
    if (s == QStringLiteral("black"))      return FontWeight::Black;
    return FontWeight::Normal;
}
static QString fontWeightStr(FontWeight v)
{
    switch (v) {
    case FontWeight::Thin:       return QStringLiteral("thin");
    case FontWeight::ExtraLight: return QStringLiteral("extraLight");
    case FontWeight::Light:      return QStringLiteral("light");
    case FontWeight::SemiLight:  return QStringLiteral("semiLight");
    case FontWeight::Medium:     return QStringLiteral("medium");
    case FontWeight::SemiBold:   return QStringLiteral("semiBold");
    case FontWeight::Bold:       return QStringLiteral("bold");
    case FontWeight::ExtraBold:  return QStringLiteral("extraBold");
    case FontWeight::Black:      return QStringLiteral("black");
    default:                     return QStringLiteral("normal");
    }
}

static IntenseTextStyle parseIntenseTextStyle(const QString& s)
{
    if (s == QStringLiteral("bold")) return IntenseTextStyle::Bold;
    if (s == QStringLiteral("all"))  return IntenseTextStyle::All;
    return IntenseTextStyle::Bright;
}
static QString intenseTextStyleStr(IntenseTextStyle v)
{
    switch (v) {
    case IntenseTextStyle::Bold: return QStringLiteral("bold");
    case IntenseTextStyle::All:  return QStringLiteral("all");
    default:                     return QStringLiteral("bright");
    }
}

static BgStretchMode parseBgStretchMode(const QString& s)
{
    if (s == QStringLiteral("none"))         return BgStretchMode::None;
    if (s == QStringLiteral("fill"))         return BgStretchMode::Fill;
    if (s == QStringLiteral("uniform"))      return BgStretchMode::Uniform;
    return BgStretchMode::UniformToFill;
}
static QString bgStretchModeStr(BgStretchMode v)
{
    switch (v) {
    case BgStretchMode::None:    return QStringLiteral("none");
    case BgStretchMode::Fill:    return QStringLiteral("fill");
    case BgStretchMode::Uniform: return QStringLiteral("uniform");
    default:                     return QStringLiteral("uniformToFill");
    }
}

static BgAlignment parseBgAlignment(const QString& s)
{
    if (s == QStringLiteral("topLeft"))      return BgAlignment::TopLeft;
    if (s == QStringLiteral("topCenter"))    return BgAlignment::TopCenter;
    if (s == QStringLiteral("topRight"))     return BgAlignment::TopRight;
    if (s == QStringLiteral("middleLeft"))   return BgAlignment::MiddleLeft;
    if (s == QStringLiteral("middleRight"))  return BgAlignment::MiddleRight;
    if (s == QStringLiteral("bottomLeft"))   return BgAlignment::BottomLeft;
    if (s == QStringLiteral("bottomCenter")) return BgAlignment::BottomCenter;
    if (s == QStringLiteral("bottomRight"))  return BgAlignment::BottomRight;
    return BgAlignment::MiddleCenter;
}
static QString bgAlignmentStr(BgAlignment v)
{
    switch (v) {
    case BgAlignment::TopLeft:      return QStringLiteral("topLeft");
    case BgAlignment::TopCenter:    return QStringLiteral("topCenter");
    case BgAlignment::TopRight:     return QStringLiteral("topRight");
    case BgAlignment::MiddleLeft:   return QStringLiteral("middleLeft");
    case BgAlignment::MiddleRight:  return QStringLiteral("middleRight");
    case BgAlignment::BottomLeft:   return QStringLiteral("bottomLeft");
    case BgAlignment::BottomCenter: return QStringLiteral("bottomCenter");
    case BgAlignment::BottomRight:  return QStringLiteral("bottomRight");
    default:                        return QStringLiteral("middleCenter");
    }
}

// ── Optional color helpers ────────────────────────────────────────────────────

static std::optional<QColor> readOptionalColor(const QJsonObject& obj, const QString& key)
{
    const QJsonValue v = obj[key];
    if (v.isString()) {
        const QColor c(v.toString());
        if (c.isValid()) return c;
    }
    return std::nullopt;
}

static void writeOptionalColor(QJsonObject& obj, const QString& key,
                               const std::optional<QColor>& c)
{
    if (c.has_value()) {
        obj[key] = c->name(QColor::HexRgb);
    }
}

// ── fromJson ──────────────────────────────────────────────────────────────────

Profile Profile::fromJson(const QJsonObject& obj)
{
    Profile p;
    p.name             = obj[QStringLiteral("name")].toString(p.name);
    p.guid             = obj[QStringLiteral("guid")].toString(p.guid);
    if (p.guid.isEmpty()) {
        p.guid = QUuid::createUuid().toString(QUuid::WithBraces);
    }

    // General
    p.shell              = obj[QStringLiteral("commandline")].toString(p.shell);
    p.workingDirectory   = obj[QStringLiteral("startingDirectory")].toString(p.workingDirectory);
    p.tabTitle           = obj[QStringLiteral("tabTitle")].toString(p.tabTitle);
    p.tabColor           = obj[QStringLiteral("tabColor")].toString(p.tabColor);
    p.hidden             = obj[QStringLiteral("hidden")].toBool(p.hidden);
    p.suppressApplicationTitle = obj[QStringLiteral("suppressApplicationTitle")].toBool(p.suppressApplicationTitle);
    p.closeOnExit        = parseCloseOnExit(obj[QStringLiteral("closeOnExit")].toString());

    // Font
    p.fontFamily         = obj[QStringLiteral("font")].toString(p.fontFamily);
    p.fontSize           = obj[QStringLiteral("fontSize")].toInt(p.fontSize);
    p.fontWeight         = parseFontWeight(obj[QStringLiteral("fontWeight")].toString());
    p.enableBuiltinGlyphs= obj[QStringLiteral("enableBuiltinGlyphs")].toBool(p.enableBuiltinGlyphs);
    p.enableColorGlyphs  = obj[QStringLiteral("enableColorGlyphs")].toBool(p.enableColorGlyphs);

    // Appearance
    p.colorScheme        = obj[QStringLiteral("colorScheme")].toString(p.colorScheme);
    p.cursorShape        = parseCursorShape(obj[QStringLiteral("cursorShape")].toString());
    p.cursorHeight       = obj[QStringLiteral("cursorHeight")].toInt(p.cursorHeight);
    p.cursorColor        = readOptionalColor(obj, QStringLiteral("cursorColor"));
    p.foregroundColorOverride = readOptionalColor(obj, QStringLiteral("foreground"));
    p.backgroundColorOverride = readOptionalColor(obj, QStringLiteral("background"));
    p.selectionBackground     = readOptionalColor(obj, QStringLiteral("selectionBackground"));
    p.opacity              = obj[QStringLiteral("opacity")].toDouble(p.opacity);
    p.retroTerminalEffect  = obj[QStringLiteral("experimental.retroTerminalEffect")].toBool(p.retroTerminalEffect);
    p.intenseTextStyle     = parseIntenseTextStyle(obj[QStringLiteral("intenseTextStyle")].toString());
    p.adjustIndistinguishableColors = obj[QStringLiteral("adjustIndistinguishableColors")].toBool(p.adjustIndistinguishableColors);

    // Background image — accept both old "backgroundOpacity" key and new key
    p.backgroundImagePath    = obj[QStringLiteral("backgroundImage")].toString(p.backgroundImagePath);
    p.backgroundImageOpacity = obj[QStringLiteral("backgroundImageOpacity")].toDouble(
        obj[QStringLiteral("backgroundOpacity")].toDouble(p.backgroundImageOpacity));
    p.backgroundImageStretchMode = parseBgStretchMode(obj[QStringLiteral("backgroundImageStretchMode")].toString());
    p.backgroundImageAlignment   = parseBgAlignment(obj[QStringLiteral("backgroundImageAlignment")].toString());

    // Advanced
    p.historySize   = obj[QStringLiteral("historySize")].toInt(p.historySize);
    p.scrollbarState= parseScrollbarState(obj[QStringLiteral("scrollbarState")].toString());
    p.padding       = obj[QStringLiteral("padding")].toInt(p.padding);
    p.snapOnInput   = obj[QStringLiteral("snapOnInput")].toBool(p.snapOnInput);
    p.altGrAliasing = obj[QStringLiteral("altGrAliasing")].toBool(p.altGrAliasing);
    p.bellStyle     = parseBellStyle(obj[QStringLiteral("bellStyle")].toString());

    const QJsonObject envObj = obj[QStringLiteral("environmentVariables")].toObject();
    for (const QString& key : envObj.keys()) {
        p.environmentVariables[key] = envObj[key].toString();
    }

    return p;
}

// ── toJson ────────────────────────────────────────────────────────────────────

QJsonObject Profile::toJson() const
{
    QJsonObject obj;
    obj[QStringLiteral("name")] = name;
    obj[QStringLiteral("guid")] = guid;

    // General
    if (!shell.isEmpty())            obj[QStringLiteral("commandline")]          = shell;
    if (!workingDirectory.isEmpty()) obj[QStringLiteral("startingDirectory")]    = workingDirectory;
    if (!tabTitle.isEmpty())         obj[QStringLiteral("tabTitle")]             = tabTitle;
    if (!tabColor.isEmpty())         obj[QStringLiteral("tabColor")]             = tabColor;
    if (hidden)                      obj[QStringLiteral("hidden")]               = true;
    if (suppressApplicationTitle)    obj[QStringLiteral("suppressApplicationTitle")] = true;
    obj[QStringLiteral("closeOnExit")] = closeOnExitStr(closeOnExit);

    // Font
    obj[QStringLiteral("font")]               = fontFamily;
    obj[QStringLiteral("fontSize")]           = fontSize;
    obj[QStringLiteral("fontWeight")]         = fontWeightStr(fontWeight);
    obj[QStringLiteral("enableBuiltinGlyphs")]= enableBuiltinGlyphs;
    obj[QStringLiteral("enableColorGlyphs")]  = enableColorGlyphs;

    // Appearance
    obj[QStringLiteral("colorScheme")]    = colorScheme;
    obj[QStringLiteral("cursorShape")]    = cursorShapeStr(cursorShape);
    obj[QStringLiteral("cursorHeight")]   = cursorHeight;
    writeOptionalColor(obj, QStringLiteral("cursorColor"), cursorColor);
    writeOptionalColor(obj, QStringLiteral("foreground"), foregroundColorOverride);
    writeOptionalColor(obj, QStringLiteral("background"), backgroundColorOverride);
    writeOptionalColor(obj, QStringLiteral("selectionBackground"), selectionBackground);
    obj[QStringLiteral("opacity")]        = opacity;
    obj[QStringLiteral("experimental.retroTerminalEffect")] = retroTerminalEffect;
    obj[QStringLiteral("intenseTextStyle")] = intenseTextStyleStr(intenseTextStyle);
    obj[QStringLiteral("adjustIndistinguishableColors")] = adjustIndistinguishableColors;

    // Background image
    if (!backgroundImagePath.isEmpty()) {
        obj[QStringLiteral("backgroundImage")]              = backgroundImagePath;
        obj[QStringLiteral("backgroundImageOpacity")]       = backgroundImageOpacity;
        obj[QStringLiteral("backgroundImageStretchMode")]   = bgStretchModeStr(backgroundImageStretchMode);
        obj[QStringLiteral("backgroundImageAlignment")]     = bgAlignmentStr(backgroundImageAlignment);
    }

    // Advanced
    obj[QStringLiteral("historySize")]   = historySize;
    obj[QStringLiteral("scrollbarState")]= scrollbarStateStr(scrollbarState);
    obj[QStringLiteral("padding")]       = padding;
    obj[QStringLiteral("snapOnInput")]   = snapOnInput;
    obj[QStringLiteral("altGrAliasing")] = altGrAliasing;
    obj[QStringLiteral("bellStyle")]     = bellStyleStr(bellStyle);

    if (!environmentVariables.isEmpty()) {
        QJsonObject envObj;
        for (auto it = environmentVariables.constBegin(); it != environmentVariables.constEnd(); ++it) {
            envObj[it.key()] = it.value();
        }
        obj[QStringLiteral("environmentVariables")] = envObj;
    }

    return obj;
}

Profile Profile::Default()
{
    Profile p;
    p.guid = QUuid::createUuid().toString(QUuid::WithBraces);
    return p;
}

} // namespace LTerm
