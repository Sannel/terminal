// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "TerminalWidget.hpp"
#include "KTermSettings.hpp"

#include <KXmlGuiWindow>
#include <QTabWidget>

namespace KTerm {

/**
 * MainWindow — top-level KDE window with a tab bar for multiple terminals.
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void newTab(const QString& profileName = {});
    void closeCurrentTab();

private slots:
    void _onTabTitleChanged(const QString& title);
    void _onTabCloseRequested(int index);

private:
    TerminalWidget* _currentTerminal() const;
    TerminalWidget* _terminalAt(int index) const;
    void _setupActions();

    QTabWidget* _tabs = nullptr;
};

} // namespace KTerm
