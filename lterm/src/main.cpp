// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "MainWindow.hpp"

#include <KAboutData>
#include <KLocalizedString>
#include <LTermSettings.hpp>
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    KAboutData about(
        QStringLiteral("lterm"),
        i18n("LTerm"),
        QStringLiteral("0.1.0"),
        i18n("A modern terminal for KDE Plasma"),
        KAboutLicense::MIT,
        i18n("© 2025 Sannel LLC"));
    about.addAuthor(i18n("Sannel Contributors"));
    KAboutData::setApplicationData(about);

    LTerm::MainWindow window;

    // Apply launch mode before show.
    using LTerm::LaunchMode;
    const auto lm = LTerm::LTermSettings::instance().global().launchMode;

    switch (lm) {
    case LaunchMode::Maximized:
        window.showMaximized();
        break;
    case LaunchMode::Fullscreen:
        window.showFullScreen();
        break;
    default:
        window.show();
        break;
    }

    return app.exec();
}


