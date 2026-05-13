// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "MainWindow.hpp"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    KAboutData about(
        QStringLiteral("kterm"),
        i18n("KTerm"),
        QStringLiteral("0.1.0"),
        i18n("A modern terminal for KDE Plasma"),
        KAboutLicense::MIT,
        i18n("© 2025 Sannel LLC"));
    about.addAuthor(i18n("Sannel Contributors"));
    KAboutData::setApplicationData(about);

    KTerm::MainWindow window;
    window.show();

    return app.exec();
}


