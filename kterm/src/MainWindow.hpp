// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "TerminalWidget.hpp"
#include "KTermSettings.hpp"

#include <KXmlGuiWindow>
#include <QTabWidget>
#include <QToolButton>

namespace KTerm {

class SettingsDialog;

/**
 * MainWindow — KDE main window with a tab bar for multiple terminals.
 *
 * No menu bar. A corner widget on the tab bar provides:
 *   [+▾] — new tab (left-click: default profile; arrow: choose shell/profile)
 *   [≡]  — app menu (settings, about, quit)
 */
class MainWindow : public KXmlGuiWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

public slots:
    void newTab(const QString& profileName = {});
    void newTabWithShell(const QString& shellPath);
    void closeCurrentTab();

private slots:
    void _onTabCloseRequested(int index);
    void _openSettings();

private:
    void _setupActions();
    void _setupCornerWidget();

    QMenu* _buildNewTabMenu();
    void   _populateNewTabMenu(QMenu* menu);
    QMenu* _buildAppMenu();

    void _startTab(const Profile& profile, const ColorScheme& scheme);

    TerminalWidget* _currentTerminal() const;

    QTabWidget*  _tabs      = nullptr;
    QToolButton* _newTabBtn = nullptr;
    QToolButton* _menuBtn   = nullptr;
};

} // namespace KTerm

