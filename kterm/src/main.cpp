// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "TerminalWidget.hpp"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <KMainWindow>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    KAboutData about(
        QStringLiteral("kterm"),
        i18n("KTerm"),
        QStringLiteral("0.1.0"),
        i18n("A modern GPU-accelerated terminal for KDE Plasma"),
        KAboutLicense::MIT,
        i18n("© 2025 Sannel LLC"));
    about.addAuthor(i18n("Sannel Contributors"));
    KAboutData::setApplicationData(about);

    KMainWindow window;
    window.setWindowTitle(i18n("KTerm"));
    window.resize(900, 600);

    auto* term = new KTerm::TerminalWidget(&window);
    window.setCentralWidget(term);
    window.show();

    term->Start();

    return app.exec();
}

