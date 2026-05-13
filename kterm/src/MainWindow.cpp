// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "MainWindow.hpp"

#include <KActionCollection>
#include <KLocalizedString>
#include <QAction>
#include <QKeySequence>
#include <QTabBar>

namespace KTerm {

MainWindow::MainWindow(QWidget* parent) : KXmlGuiWindow(parent)
{
    setWindowTitle(i18n("KTerm"));
    resize(900, 600);

    _tabs = new QTabWidget(this);
    _tabs->setTabsClosable(true);
    _tabs->setMovable(true);
    _tabs->setDocumentMode(true);
    _tabs->setElideMode(Qt::ElideRight);

    connect(_tabs, &QTabWidget::tabCloseRequested,
            this, &MainWindow::_onTabCloseRequested);

    setCentralWidget(_tabs);

    _setupActions();

    // Set up the GUI (menus, toolbars) without requiring an XML file.
    setupGUI(Default, QString{});

    // Load settings and open a tab with the default profile.
    KTermSettings::instance().load();
    newTab();
}

// ── Actions ───────────────────────────────────────────────────────────────────

void MainWindow::_setupActions()
{
    auto* ac = actionCollection();

    auto* newTabAct = new QAction(i18n("New &Tab"), this);
    newTabAct->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    ac->addAction(QStringLiteral("new-tab"), newTabAct);
    ac->setDefaultShortcut(newTabAct, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_T));
    connect(newTabAct, &QAction::triggered, this, [this]() { newTab(); });

    auto* closeTabAct = new QAction(i18n("&Close Tab"), this);
    closeTabAct->setIcon(QIcon::fromTheme(QStringLiteral("tab-close")));
    ac->addAction(QStringLiteral("close-tab"), closeTabAct);
    ac->setDefaultShortcut(closeTabAct, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_W));
    connect(closeTabAct, &QAction::triggered, this, &MainWindow::closeCurrentTab);

    auto* nextTabAct = new QAction(i18n("&Next Tab"), this);
    ac->addAction(QStringLiteral("next-tab"), nextTabAct);
    ac->setDefaultShortcut(nextTabAct, QKeySequence(Qt::CTRL | Qt::Key_Tab));
    connect(nextTabAct, &QAction::triggered, this, [this]() {
        const int next = (_tabs->currentIndex() + 1) % _tabs->count();
        _tabs->setCurrentIndex(next);
    });

    auto* prevTabAct = new QAction(i18n("&Previous Tab"), this);
    ac->addAction(QStringLiteral("prev-tab"), prevTabAct);
    ac->setDefaultShortcut(prevTabAct, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));
    connect(prevTabAct, &QAction::triggered, this, [this]() {
        const int prev = (_tabs->currentIndex() - 1 + _tabs->count()) % _tabs->count();
        _tabs->setCurrentIndex(prev);
    });

    // Ctrl+1..9 select tabs.
    for (int i = 0; i < 9; ++i) {
        auto* act = new QAction(this);
        ac->addAction(QStringLiteral("select-tab-%1").arg(i + 1), act);
        ac->setDefaultShortcut(act, QKeySequence(Qt::CTRL | (Qt::Key_1 + i)));
        connect(act, &QAction::triggered, this, [this, i]() {
            if (i < _tabs->count()) {
                _tabs->setCurrentIndex(i);
            }
        });
    }
}

// ── Tab management ────────────────────────────────────────────────────────────

void MainWindow::newTab(const QString& profileName)
{
    auto& settings = KTermSettings::instance();
    const Profile profile = profileName.isEmpty()
        ? settings.defaultProfile()
        : settings.profile(profileName);
    const ColorScheme scheme = settings.colorScheme(profile.colorScheme);

    auto* term = new TerminalWidget(this);
    term->applyProfile(profile, scheme);

    const int idx = _tabs->addTab(term, i18n("Terminal"));
    _tabs->setCurrentIndex(idx);

    connect(term, &TerminalWidget::titleChanged, this, [this, term](const QString& title) {
        const int idx = _tabs->indexOf(term);
        if (idx >= 0) {
            _tabs->setTabText(idx, title.isEmpty() ? i18n("Terminal") : title);
        }
        if (_tabs->currentWidget() == term) {
            setWindowTitle(title.isEmpty() ? i18n("KTerm") : title);
        }
    });

    connect(term->terminal(), &KTerminal::terminated, this, [this, term]() {
        const int idx = _tabs->indexOf(term);
        if (idx >= 0) {
            _tabs->removeTab(idx);
            term->deleteLater();
        }
        if (_tabs->count() == 0) {
            close();
        }
    });

    term->Start(profile.shell);
    term->setFocus();
}

void MainWindow::closeCurrentTab()
{
    _onTabCloseRequested(_tabs->currentIndex());
}

void MainWindow::_onTabCloseRequested(int index)
{
    if (index < 0 || index >= _tabs->count()) {
        return;
    }
    auto* w = _tabs->widget(index);
    _tabs->removeTab(index);
    w->deleteLater();

    if (_tabs->count() == 0) {
        close();
    }
}

void MainWindow::_onTabTitleChanged(const QString& title)
{
    // Handled inline via lambdas in newTab().
    Q_UNUSED(title);
}

// ── Helpers ───────────────────────────────────────────────────────────────────

TerminalWidget* MainWindow::_currentTerminal() const
{
    return qobject_cast<TerminalWidget*>(_tabs->currentWidget());
}

TerminalWidget* MainWindow::_terminalAt(int index) const
{
    return qobject_cast<TerminalWidget*>(_tabs->widget(index));
}

} // namespace KTerm
