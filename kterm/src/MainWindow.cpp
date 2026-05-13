// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "MainWindow.hpp"
#include "SettingsDialog.hpp"

#include <KLocalizedString>

#include <QAction>
#include <QApplication>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QShortcut>
#include <QStandardPaths>
#include <QTabBar>

namespace KTerm {

MainWindow::MainWindow(QWidget* parent) : KXmlGuiWindow(parent)
{
    setWindowTitle(i18n("KTerm"));

    _tabs = new QTabWidget(this);
    _tabs->setTabsClosable(true);
    _tabs->setMovable(true);
    _tabs->setDocumentMode(true);
    _tabs->setElideMode(Qt::ElideRight);
    connect(_tabs, &QTabWidget::tabCloseRequested,
            this, &MainWindow::_onTabCloseRequested);
    setCentralWidget(_tabs);

    _setupActions();
    _setupCornerWidget();

    // Geometry / state saving only — no menu bar, no toolbar created.
    setupGUI(Save, QString{});
    menuBar()->hide();

    KTermSettings::instance().load();
    newTab();
}

// ── Actions (keyboard shortcuts, no menu bar) ────────────────────────────────

void MainWindow::_setupActions()
{
    auto shortcut = [this](QKeySequence key, auto slot) {
        auto* sc = new QShortcut(key, this);
        connect(sc, &QShortcut::activated, this, slot);
    };

    shortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T), [this]() { newTab(); });
    shortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W), [this]() { closeCurrentTab(); });
    shortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), [this]() {
        _tabs->setCurrentIndex((_tabs->currentIndex() + 1) % _tabs->count());
    });
    shortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), [this]() {
        _tabs->setCurrentIndex((_tabs->currentIndex() - 1 + _tabs->count()) % _tabs->count());
    });
    for (int i = 0; i < 9; ++i) {
        shortcut(QKeySequence(Qt::CTRL | (Qt::Key_1 + i)), [this, i]() {
            if (i < _tabs->count()) _tabs->setCurrentIndex(i);
        });
    }
}

// ── Corner widget ────────────────────────────────────────────────────────────

void MainWindow::_setupCornerWidget()
{
    auto* corner = new QWidget(this);
    auto* layout = new QHBoxLayout(corner);
    layout->setContentsMargins(2, 0, 4, 0);
    layout->setSpacing(2);

    _newTabBtn = new QToolButton(corner);
    _newTabBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    _newTabBtn->setToolTip(i18n("New Terminal\nLeft-click: default profile  |  Arrow: choose shell"));
    _newTabBtn->setPopupMode(QToolButton::MenuButtonPopup);
    _newTabBtn->setAutoRaise(true);
    _newTabBtn->setMenu(_buildNewTabMenu());
    connect(_newTabBtn, &QToolButton::clicked, this, [this]() { newTab(); });
    layout->addWidget(_newTabBtn);

    _menuBtn = new QToolButton(corner);
    _menuBtn->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    _menuBtn->setToolTip(i18n("Menu"));
    _menuBtn->setPopupMode(QToolButton::InstantPopup);
    _menuBtn->setAutoRaise(true);
    _menuBtn->setMenu(_buildAppMenu());
    layout->addWidget(_menuBtn);

    _tabs->setCornerWidget(corner, Qt::TopRightCorner);
}

// ── New-tab menu ─────────────────────────────────────────────────────────────

QMenu* MainWindow::_buildNewTabMenu()
{
    auto* menu = new QMenu(this);
    connect(menu, &QMenu::aboutToShow, this, [this, menu]() {
        menu->clear();
        _populateNewTabMenu(menu);
    });
    return menu;
}

void MainWindow::_populateNewTabMenu(QMenu* menu)
{
    auto& s = KTermSettings::instance();

    // Saved profiles
    for (auto it = s.profiles().constBegin(); it != s.profiles().constEnd(); ++it) {
        const QString& name    = it.key();
        const Profile& profile = it.value();
        const QString shellName = profile.shell.isEmpty()
            ? QStringLiteral("$SHELL")
            : QFileInfo(profile.shell).fileName();
        auto* act = menu->addAction(
            QIcon::fromTheme(QStringLiteral("utilities-terminal")),
            QStringLiteral("%1  (%2)").arg(name, shellName));
        connect(act, &QAction::triggered, this, [this, n = name]() { newTab(n); });
    }

    // Quick-launch shells not already covered by a profile
    QSet<QString> profileShells;
    for (const auto& p : s.profiles()) {
        if (!p.shell.isEmpty()) profileShells.insert(QFileInfo(p.shell).fileName());
    }

    static const QList<QPair<QString, QString>> candidates = {
        {"bash",  QStringLiteral("Bash")},
        {"zsh",   QStringLiteral("Zsh")},
        {"fish",  QStringLiteral("Fish Shell")},
        {"pwsh",  QStringLiteral("PowerShell")},
        {"sh",    QStringLiteral("sh")},
    };

    bool addedShell = false;
    menu->addSeparator();
    for (const auto& [exe, label] : candidates) {
        const QString path = QStandardPaths::findExecutable(exe);
        if (path.isEmpty() || profileShells.contains(exe)) continue;
        auto* act = menu->addAction(
            QIcon::fromTheme(QStringLiteral("utilities-terminal")), label);
        connect(act, &QAction::triggered, this, [this, p = path]() { newTabWithShell(p); });
        addedShell = true;
    }

    if (!addedShell) {
        // Remove the dangling separator if no extra shells were added.
        const auto actions = menu->actions();
        if (!actions.isEmpty() && actions.last()->isSeparator()) {
            delete actions.last();
        }
    }

    menu->addSeparator();
    auto* settingsAct = menu->addAction(
        QIcon::fromTheme(QStringLiteral("configure")), i18n("Settings…"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::_openSettings);
}

// ── App menu ──────────────────────────────────────────────────────────────────

QMenu* MainWindow::_buildAppMenu()
{
    auto* menu = new QMenu(this);

    auto* settingsAct = menu->addAction(
        QIcon::fromTheme(QStringLiteral("configure")), i18n("Settings…"));
    connect(settingsAct, &QAction::triggered, this, &MainWindow::_openSettings);

    menu->addSeparator();

    auto* aboutAct = menu->addAction(
        QIcon::fromTheme(QStringLiteral("help-about")), i18n("About KTerm"));
    connect(aboutAct, &QAction::triggered, this, []() {
        QMessageBox::about(nullptr, QStringLiteral("About KTerm"),
            QStringLiteral("<h3>KTerm</h3>"
                           "<p>A KDE Plasma terminal emulator.<br/>"
                           "Built on Qt&nbsp;6 and KDE&nbsp;Frameworks&nbsp;6.</p>"));
    });

    menu->addSeparator();

    auto* quitAct = menu->addAction(
        QIcon::fromTheme(QStringLiteral("application-exit")), i18n("Quit"));
    connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);

    return menu;
}

// ── Settings ──────────────────────────────────────────────────────────────────

void MainWindow::_openSettings()
{
    SettingsDialog dlg(this);
    dlg.exec();
}

// ── Tab management ────────────────────────────────────────────────────────────

void MainWindow::newTab(const QString& profileName)
{
    auto& s = KTermSettings::instance();
    const Profile     profile = profileName.isEmpty() ? s.defaultProfile() : s.profile(profileName);
    const ColorScheme scheme  = s.colorScheme(profile.colorScheme);
    _startTab(profile, scheme);
}

void MainWindow::newTabWithShell(const QString& shellPath)
{
    auto& s = KTermSettings::instance();
    Profile p = s.defaultProfile();
    p.shell = shellPath;
    const ColorScheme scheme = s.colorScheme(p.colorScheme);
    _startTab(p, scheme);
}

void MainWindow::_startTab(const Profile& profile, const ColorScheme& scheme)
{
    auto* term = new TerminalWidget(this);
    term->applyProfile(profile, scheme);

    const int idx = _tabs->addTab(term, i18n("Terminal"));
    _tabs->setCurrentIndex(idx);

    connect(term, &TerminalWidget::titleChanged, this, [this, term](const QString& title) {
        const int i = _tabs->indexOf(term);
        if (i >= 0) _tabs->setTabText(i, title.isEmpty() ? i18n("Terminal") : title);
        if (_tabs->currentWidget() == term)
            setWindowTitle(title.isEmpty() ? i18n("KTerm") : title);
    });

    connect(term->terminal(), &KTerminal::terminated, this, [this, term]() {
        const int i = _tabs->indexOf(term);
        if (i >= 0) _tabs->removeTab(i);
        term->deleteLater();
        if (_tabs->count() == 0) close();
    });

    term->Start(profile.shell, {}, profile.workingDirectory);
    term->setFocus();
}

void MainWindow::closeCurrentTab()
{
    _onTabCloseRequested(_tabs->currentIndex());
}

void MainWindow::_onTabCloseRequested(int index)
{
    if (index < 0 || index >= _tabs->count()) return;
    auto* w = _tabs->widget(index);
    _tabs->removeTab(index);
    w->deleteLater();
    if (_tabs->count() == 0) close();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

TerminalWidget* MainWindow::_currentTerminal() const
{
    return qobject_cast<TerminalWidget*>(_tabs->currentWidget());
}

} // namespace KTerm

