// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "SettingsDialog.hpp"

#include <KLocalizedString>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QTableWidget>
#include <QVBoxLayout>

namespace LTerm {

// ── Nav item data roles ───────────────────────────────────────────────────────

static constexpr int NavTypeRole = Qt::UserRole;
static constexpr int NavDataRole = Qt::UserRole + 1;

enum NavItemType {
    NavHeader       = 0,
    NavStartup      = 1,
    NavInteraction  = 2,
    NavAppearanceGlobal = 3,
    NavProfile      = 4,
    NavAddProfile   = 5,
    NavColorSchemes = 6,
};

// ── Color button helpers ──────────────────────────────────────────────────────

void SettingsDialog::_updateColorBtn(QPushButton* btn, const std::optional<QColor>& c)
{
    if (c.has_value()) {
        const QColor& col = *c;
        const QString contrast = (col.lightness() > 128) ? QStringLiteral("black")
                                                          : QStringLiteral("white");
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { background-color: %1; color: %2; border: 1px solid palette(mid);"
            " border-radius: 3px; padding: 2px 8px; }")
            .arg(col.name(QColor::HexRgb), contrast));
        btn->setText(col.name(QColor::HexRgb));
    } else {
        btn->setStyleSheet(QStringLiteral(
            "QPushButton { border: 1px solid palette(mid); border-radius: 3px; padding: 2px 8px; }"));
        btn->setText(i18n("Default"));
    }
}

std::optional<QColor> SettingsDialog::_pickColor(QPushButton* /*btn*/,
                                                  const std::optional<QColor>& current)
{
    QColor initial = current.value_or(Qt::white);
    const QColor picked = QColorDialog::getColor(initial, nullptr,
        i18n("Choose Color"), QColorDialog::ShowAlphaChannel);
    if (picked.isValid()) return picked;
    return current;
}

void SettingsDialog::_connectColorBtn(QPushButton* btn, std::optional<QColor>& storage)
{
    connect(btn, &QPushButton::clicked, this, [this, btn, &storage]() {
        const auto picked = _pickColor(btn, storage);
        if (picked != storage) {
            storage = picked;
            _updateColorBtn(btn, storage);
        }
    });
    btn->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(btn, &QPushButton::customContextMenuRequested, this, [btn, &storage]() {
        storage = std::nullopt;
        _updateColorBtn(btn, storage);
    });
    btn->setToolTip(i18n("Left-click to pick a color. Right-click to reset to Default."));
    _updateColorBtn(btn, storage);
}

// ── Constructor ───────────────────────────────────────────────────────────────

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(i18n("LTerm Settings"));
    setMinimumSize(880, 600);
    resize(940, 640);

    auto& s = LTermSettings::instance();
    _localProfiles = s.profiles();
    _localGlobal   = s.global();

    // ── Top-level layout: nav | stack ────────────────────────────────────────
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Left sidebar ─────────────────────────────────────────────────────────
    auto* sidePanel = new QWidget(this);
    sidePanel->setFixedWidth(220);
    sidePanel->setObjectName(QStringLiteral("sidePanel"));
    sidePanel->setStyleSheet(QStringLiteral(
        "#sidePanel { background: palette(dark); border-right: 1px solid palette(mid); }"));
    auto* sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setContentsMargins(0, 8, 0, 8);
    sideLayout->setSpacing(0);

    _nav = new QListWidget(sidePanel);
    _nav->setFrameShape(QFrame::NoFrame);
    _nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    _nav->setStyleSheet(QStringLiteral(R"(
        QListWidget {
            background: transparent;
            border: none;
            outline: none;
        }
        QListWidget::item {
            padding: 6px 12px;
            border-radius: 4px;
            margin: 1px 6px;
        }
        QListWidget::item:selected {
            background: palette(highlight);
            color: palette(highlighted-text);
        }
        QListWidget::item:hover:!selected {
            background: palette(midlight);
        }
    )"));
    sideLayout->addWidget(_nav);
    root->addWidget(sidePanel);

    // ── Right content area ────────────────────────────────────────────────────
    auto* rightPanel = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    _stack = new QStackedWidget(rightPanel);
    rightLayout->addWidget(_stack, 1);

    // Bottom bar: Save + Cancel
    auto* bottomBar = new QWidget(rightPanel);
    bottomBar->setObjectName(QStringLiteral("bottomBar"));
    bottomBar->setStyleSheet(QStringLiteral(
        "#bottomBar { border-top: 1px solid palette(mid); }"));
    auto* bottomLayout = new QHBoxLayout(bottomBar);
    bottomLayout->setContentsMargins(16, 8, 16, 8);
    bottomLayout->addStretch();
    _saveBtn = new QPushButton(i18n("Save"), bottomBar);
    _saveBtn->setDefault(true);
    auto* cancelBtn = new QPushButton(i18n("Cancel"), bottomBar);
    bottomLayout->addWidget(cancelBtn);
    bottomLayout->addWidget(_saveBtn);
    rightLayout->addWidget(bottomBar);

    root->addWidget(rightPanel, 1);

    // ── Build nav and pages ───────────────────────────────────────────────────
    _buildNav();
    _buildStartupPage();         // index 0
    _buildInteractionPage();     // index 1
    _buildGlobalAppearancePage();// index 2
    _buildProfilePage();         // index 3
    _buildColorSchemesPage();    // index 4

    // ── Connections ───────────────────────────────────────────────────────────
    connect(_nav, &QListWidget::itemClicked, this, &SettingsDialog::_onNavItemClicked);
    connect(_saveBtn,  &QPushButton::clicked, this, &SettingsDialog::_save);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Load global settings into forms
    _loadGlobalToStartup();
    _loadGlobalToInteraction();
    _loadGlobalToAppearance();

    // Select "Startup" on open
    for (int i = 0; i < _nav->count(); ++i) {
        auto* item = _nav->item(i);
        if (item->data(NavTypeRole).toInt() == NavStartup) {
            _nav->setCurrentItem(item);
            _stack->setCurrentIndex(0);
            break;
        }
    }
}

// ── Nav building ──────────────────────────────────────────────────────────────

QListWidgetItem* SettingsDialog::_addNavHeader(const QString& text)
{
    auto* item = new QListWidgetItem(text.toUpper(), _nav);
    item->setData(NavTypeRole, NavHeader);
    item->setFlags(Qt::NoItemFlags);
    QFont f = item->font();
    f.setPointSizeF(f.pointSizeF() * 0.78);
    f.setBold(true);
    f.setLetterSpacing(QFont::PercentageSpacing, 115);
    item->setFont(f);
    item->setForeground(_nav->palette().color(QPalette::Mid));
    return item;
}

QListWidgetItem* SettingsDialog::_addNavItem(const QString& text, const QIcon& icon,
                                              const QString& data, int indent)
{
    const QString display = indent > 0 ? QStringLiteral("   ") + text : text;
    auto* item = new QListWidgetItem(icon, display, _nav);
    item->setData(NavDataRole, data);
    return item;
}

void SettingsDialog::_buildNav()
{
    _nav->clear();

    auto* startupItem = _addNavItem(
        i18n("Startup"),
        QIcon::fromTheme(QStringLiteral("preferences-system")),
        QStringLiteral("startup"));
    startupItem->setData(NavTypeRole, NavStartup);

    auto* interactionItem = _addNavItem(
        i18n("Interaction"),
        QIcon::fromTheme(QStringLiteral("input-keyboard")),
        QStringLiteral("interaction"));
    interactionItem->setData(NavTypeRole, NavInteraction);

    auto* appearanceItem = _addNavItem(
        i18n("Appearance"),
        QIcon::fromTheme(QStringLiteral("preferences-desktop-theme")),
        QStringLiteral("appearance"));
    appearanceItem->setData(NavTypeRole, NavAppearanceGlobal);

    _addNavHeader(i18n("Profiles"));
    _rebuildProfileNavItems();

    auto* csItem = _addNavItem(
        i18n("Color Schemes"),
        QIcon::fromTheme(QStringLiteral("color-picker")),
        QStringLiteral("colorschemes"));
    csItem->setData(NavTypeRole, NavColorSchemes);
}

void SettingsDialog::_rebuildProfileNavItems()
{
    // Remove existing profile + add-profile items
    for (int i = _nav->count() - 1; i >= 0; --i) {
        const int t = _nav->item(i)->data(NavTypeRole).toInt();
        if (t == NavProfile || t == NavAddProfile) {
            delete _nav->takeItem(i);
        }
    }

    // Find insertion point: after the last NavHeader before NavColorSchemes
    int csIdx = _nav->count();
    for (int i = 0; i < _nav->count(); ++i) {
        if (_nav->item(i)->data(NavTypeRole).toInt() == NavColorSchemes) {
            csIdx = i;
            break;
        }
    }

    int pos = csIdx;

    // Add "+ Add a new profile" first (will be at bottom of profile section)
    auto* addItem = new QListWidgetItem(
        QIcon::fromTheme(QStringLiteral("list-add")),
        QStringLiteral("   ") + i18n("Add a new profile"));
    addItem->setData(NavTypeRole, NavAddProfile);
    _nav->insertItem(pos, addItem);

    // Insert profiles above the add item (reverse order so first profile ends up first)
    const QList<QString> names = _localProfiles.keys();
    for (int i = names.size() - 1; i >= 0; --i) {
        auto* item = new QListWidgetItem(
            QIcon::fromTheme(QStringLiteral("utilities-terminal")),
            QStringLiteral("   ") + names[i]);
        item->setData(NavTypeRole, NavProfile);
        item->setData(NavDataRole, names[i]);
        _nav->insertItem(pos, item);
    }
}

// ── Page building ─────────────────────────────────────────────────────────────

static QLabel* makeSectionTitle(const QString& text, QWidget* parent = nullptr)
{
    auto* lbl = new QLabel(text, parent);
    QFont f = lbl->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    f.setBold(true);
    lbl->setFont(f);
    return lbl;
}

void SettingsDialog::_buildStartupPage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);
    vbox->addWidget(makeSectionTitle(i18n("Startup")));

    auto* form = new QFormLayout;
    form->setSpacing(10);

    _defaultCombo = new QComboBox;
    _syncDefaultCombo();
    form->addRow(i18n("Default profile:"), _defaultCombo);

    _launchModeCombo = new QComboBox;
    _launchModeCombo->addItem(i18n("Default"),    QStringLiteral("default"));
    _launchModeCombo->addItem(i18n("Maximized"),  QStringLiteral("maximized"));
    _launchModeCombo->addItem(i18n("Fullscreen"), QStringLiteral("fullscreen"));
    _launchModeCombo->addItem(i18n("Focus"),      QStringLiteral("focus"));
    form->addRow(i18n("Launch mode:"), _launchModeCombo);

    auto* rowColRow = new QHBoxLayout;
    _initialRowsSpin = new QSpinBox; _initialRowsSpin->setRange(1, 500);
    _initialColsSpin = new QSpinBox; _initialColsSpin->setRange(1, 500);
    rowColRow->addWidget(new QLabel(i18n("Rows:")));
    rowColRow->addWidget(_initialRowsSpin);
    rowColRow->addSpacing(16);
    rowColRow->addWidget(new QLabel(i18n("Columns:")));
    rowColRow->addWidget(_initialColsSpin);
    rowColRow->addStretch();
    form->addRow(i18n("Initial size:"), rowColRow);

    _tabWidthModeCombo = new QComboBox;
    _tabWidthModeCombo->addItem(i18n("Equal"),        QStringLiteral("equal"));
    _tabWidthModeCombo->addItem(i18n("Compact"),      QStringLiteral("compact"));
    _tabWidthModeCombo->addItem(i18n("Title length"), QStringLiteral("titleLength"));
    form->addRow(i18n("Tab width mode:"), _tabWidthModeCombo);

    vbox->addLayout(form);

    _centerOnLaunchCheck  = new QCheckBox(i18n("Center window on launch"));
    _alwaysOnTopCheck     = new QCheckBox(i18n("Always show on top of other windows"));
    _alwaysShowTabsCheck  = new QCheckBox(i18n("Always show the tab bar"));
    vbox->addWidget(_centerOnLaunchCheck);
    vbox->addWidget(_alwaysOnTopCheck);
    vbox->addWidget(_alwaysShowTabsCheck);

    vbox->addStretch();
    _stack->addWidget(page); // index 0
}

void SettingsDialog::_buildInteractionPage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);
    vbox->addWidget(makeSectionTitle(i18n("Interaction")));

    _copyOnSelectCheck = new QCheckBox(i18n("Automatically copy selected text to clipboard"));
    _trimPasteCheck    = new QCheckBox(i18n("Remove trailing whitespace when pasting"));
    vbox->addWidget(_copyOnSelectCheck);
    vbox->addWidget(_trimPasteCheck);

    auto* form = new QFormLayout;
    form->setSpacing(10);
    _wordDelimitersEdit = new QLineEdit;
    _wordDelimitersEdit->setToolTip(i18n("Characters treated as word boundaries for double-click selection"));
    form->addRow(i18n("Word delimiters:"), _wordDelimitersEdit);
    vbox->addLayout(form);

    auto* note = new QLabel(i18n(
        "<i>Note: Paste warnings, URL detection, and focus-follow-mouse require "
        "additional plumbing — these settings are persisted but not yet active.</i>"));
    note->setWordWrap(true);
    note->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    vbox->addWidget(note);

    vbox->addStretch();
    _stack->addWidget(page); // index 1
}

void SettingsDialog::_buildGlobalAppearancePage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);
    vbox->addWidget(makeSectionTitle(i18n("Appearance")));

    auto* form = new QFormLayout;
    form->setSpacing(10);

    _themeCombo = new QComboBox;
    _themeCombo->addItem(i18n("Use system setting"), QStringLiteral("system"));
    _themeCombo->addItem(i18n("Dark"),               QStringLiteral("dark"));
    _themeCombo->addItem(i18n("Light"),              QStringLiteral("light"));
    _themeCombo->setToolTip(i18n("Sets the color theme for the application UI"));
    form->addRow(i18n("Application theme:"), _themeCombo);

    vbox->addLayout(form);
    vbox->addStretch();
    _stack->addWidget(page); // index 2
}

void SettingsDialog::_buildProfilePage()
{
    auto* outerPage = new QWidget;
    auto* outerLayout = new QVBoxLayout(outerPage);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);

    // Profile title bar
    auto* titleBar = new QWidget;
    auto* titleBarLayout = new QHBoxLayout(titleBar);
    titleBarLayout->setContentsMargins(32, 16, 32, 8);
    _profileTitle = new QLabel;
    QFont f = _profileTitle->font();
    f.setPointSizeF(f.pointSizeF() * 1.4);
    f.setBold(true);
    _profileTitle->setFont(f);
    titleBarLayout->addWidget(_profileTitle, 1);
    _deleteProfileBtn = new QPushButton(i18n("Delete profile"));
    _deleteProfileBtn->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));
    titleBarLayout->addWidget(_deleteProfileBtn);
    outerLayout->addWidget(titleBar);

    // Scrollable content
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);

    auto* content = new QWidget;
    auto* contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(32, 8, 32, 32);
    contentLayout->setSpacing(20);

    // ── General group ─────────────────────────────────────────────────────────
    auto* generalGroup = new QGroupBox(i18n("General"));
    auto* genForm = new QFormLayout(generalGroup);
    genForm->setSpacing(10);

    _nameEdit = new QLineEdit;
    genForm->addRow(i18n("Profile name:"), _nameEdit);

    auto* shellRow = new QHBoxLayout;
    _shellEdit = new QLineEdit;
    _shellEdit->setPlaceholderText(i18n("Default ($SHELL)"));
    auto* browseShellBtn = new QPushButton(QStringLiteral("📂"));
    browseShellBtn->setFixedWidth(30);
    browseShellBtn->setToolTip(i18n("Browse for shell executable"));
    shellRow->addWidget(_shellEdit);
    shellRow->addWidget(browseShellBtn);
    genForm->addRow(i18n("Shell command:"), shellRow);

    auto* wdRow = new QHBoxLayout;
    _workDirEdit = new QLineEdit;
    _workDirEdit->setPlaceholderText(i18n("Default (home directory)"));
    auto* browseWdBtn = new QPushButton(QStringLiteral("📂"));
    browseWdBtn->setFixedWidth(30);
    browseWdBtn->setToolTip(i18n("Browse for starting directory"));
    wdRow->addWidget(_workDirEdit);
    wdRow->addWidget(browseWdBtn);
    genForm->addRow(i18n("Starting directory:"), wdRow);

    _tabTitleEdit = new QLineEdit;
    _tabTitleEdit->setPlaceholderText(i18n("Empty = use process title"));
    genForm->addRow(i18n("Tab title:"), _tabTitleEdit);

    auto* tabColorRow = new QHBoxLayout;
    _tabColorBtn = new QPushButton;
    _tabColorBtn->setFixedWidth(120);
    _connectColorBtn(_tabColorBtn, _tabColor);
    tabColorRow->addWidget(_tabColorBtn);
    tabColorRow->addStretch();
    genForm->addRow(i18n("Tab color:"), tabColorRow);

    _hiddenCheck        = new QCheckBox(i18n("Hide profile from dropdown"));
    _suppressTitleCheck = new QCheckBox(i18n("Suppress title changes from terminal"));
    genForm->addRow(QString{}, _hiddenCheck);
    genForm->addRow(QString{}, _suppressTitleCheck);

    _closeOnExitCombo = new QComboBox;
    _closeOnExitCombo->addItem(i18n("Automatic"), QStringLiteral("automatic"));
    _closeOnExitCombo->addItem(i18n("Graceful"),  QStringLiteral("graceful"));
    _closeOnExitCombo->addItem(i18n("Always"),    QStringLiteral("always"));
    _closeOnExitCombo->addItem(i18n("Never"),     QStringLiteral("never"));
    genForm->addRow(i18n("Close on exit:"), _closeOnExitCombo);

    contentLayout->addWidget(generalGroup);

    // ── Appearance group ──────────────────────────────────────────────────────
    auto* appearGroup = new QGroupBox(i18n("Appearance"));
    auto* apForm = new QFormLayout(appearGroup);
    apForm->setSpacing(10);

    _schemeCombo = new QComboBox;
    for (const QString& name : LTermSettings::instance().colorSchemes().keys()) {
        _schemeCombo->addItem(name);
    }
    apForm->addRow(i18n("Color scheme:"), _schemeCombo);

    auto* fontRow = new QHBoxLayout;
    _fontCombo    = new QFontComboBox;
    _fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    _fontSizeSpin = new QSpinBox;
    _fontSizeSpin->setRange(6, 72);
    _fontSizeSpin->setSuffix(QStringLiteral(" pt"));
    fontRow->addWidget(_fontCombo, 1);
    fontRow->addWidget(_fontSizeSpin);
    apForm->addRow(i18n("Font:"), fontRow);

    _fontWeightCombo = new QComboBox;
    const QStringList weights = {
        i18n("Thin"), i18n("Extra Light"), i18n("Light"), i18n("Semi Light"),
        i18n("Normal"), i18n("Medium"), i18n("Semi Bold"),
        i18n("Bold"), i18n("Extra Bold"), i18n("Black")
    };
    const QStringList weightKeys = {
        QStringLiteral("thin"), QStringLiteral("extraLight"), QStringLiteral("light"),
        QStringLiteral("semiLight"), QStringLiteral("normal"), QStringLiteral("medium"),
        QStringLiteral("semiBold"), QStringLiteral("bold"),
        QStringLiteral("extraBold"), QStringLiteral("black")
    };
    for (int i = 0; i < weights.size(); ++i) {
        _fontWeightCombo->addItem(weights[i], weightKeys[i]);
    }
    apForm->addRow(i18n("Font weight:"), _fontWeightCombo);

    _cursorShapeCombo = new QComboBox;
    _cursorShapeCombo->addItem(i18n("Bar (|)"),        QStringLiteral("bar"));
    _cursorShapeCombo->addItem(i18n("Vintage (█)"),    QStringLiteral("vintage"));
    _cursorShapeCombo->addItem(i18n("Underscore (_)"), QStringLiteral("underscore"));
    _cursorShapeCombo->addItem(i18n("Filled Box (█)"), QStringLiteral("filledBox"));
    _cursorShapeCombo->addItem(i18n("Empty Box (▯)"),  QStringLiteral("emptyBox"));
    apForm->addRow(i18n("Cursor shape:"), _cursorShapeCombo);

    auto* cursorHeightRow = new QHBoxLayout;
    _cursorHeightSpin = new QSpinBox;
    _cursorHeightSpin->setRange(1, 100);
    _cursorHeightSpin->setSuffix(QStringLiteral(" %"));
    _cursorHeightSpin->setToolTip(i18n("Height of the Underscore cursor as percentage of cell height"));
    cursorHeightRow->addWidget(_cursorHeightSpin);
    cursorHeightRow->addStretch();
    apForm->addRow(i18n("Cursor height (Underscore):"), cursorHeightRow);

    auto* cursorColorRow = new QHBoxLayout;
    _cursorColorBtn = new QPushButton;
    _cursorColorBtn->setFixedWidth(120);
    _connectColorBtn(_cursorColorBtn, _cursorColor);
    cursorColorRow->addWidget(_cursorColorBtn);
    cursorColorRow->addStretch();
    apForm->addRow(i18n("Cursor color:"), cursorColorRow);

    auto* fgRow = new QHBoxLayout;
    _fgColorBtn = new QPushButton;
    _fgColorBtn->setFixedWidth(120);
    _connectColorBtn(_fgColorBtn, _fgColor);
    fgRow->addWidget(_fgColorBtn);
    fgRow->addStretch();
    apForm->addRow(i18n("Foreground color:"), fgRow);

    auto* bgRow = new QHBoxLayout;
    _bgColorBtn = new QPushButton;
    _bgColorBtn->setFixedWidth(120);
    _connectColorBtn(_bgColorBtn, _bgColor);
    bgRow->addWidget(_bgColorBtn);
    bgRow->addStretch();
    apForm->addRow(i18n("Background color:"), bgRow);

    auto* selRow = new QHBoxLayout;
    _selBgColorBtn = new QPushButton;
    _selBgColorBtn->setFixedWidth(120);
    _connectColorBtn(_selBgColorBtn, _selBgColor);
    selRow->addWidget(_selBgColorBtn);
    selRow->addStretch();
    apForm->addRow(i18n("Selection background:"), selRow);

    _enableBuiltinGlyphsCheck = new QCheckBox(i18n("Enable built-in box drawing glyphs"));
    _enableColorGlyphsCheck   = new QCheckBox(i18n("Enable color emoji and graphics"));
    apForm->addRow(QString{}, _enableBuiltinGlyphsCheck);
    apForm->addRow(QString{}, _enableColorGlyphsCheck);

    // Background image
    auto* bgImgRow = new QHBoxLayout;
    _bgImageEdit = new QLineEdit;
    _bgImageEdit->setPlaceholderText(i18n("None"));
    auto* browseBgBtn = new QPushButton(QStringLiteral("📂"));
    browseBgBtn->setFixedWidth(30);
    auto* clearBgBtn = new QPushButton(QStringLiteral("✕"));
    clearBgBtn->setFixedWidth(30);
    clearBgBtn->setToolTip(i18n("Clear background image"));
    bgImgRow->addWidget(_bgImageEdit);
    bgImgRow->addWidget(browseBgBtn);
    bgImgRow->addWidget(clearBgBtn);
    apForm->addRow(i18n("Background image:"), bgImgRow);

    auto* bgOpacityRow = new QHBoxLayout;
    _bgOpacitySpin = new QSpinBox;
    _bgOpacitySpin->setRange(0, 100);
    _bgOpacitySpin->setSuffix(QStringLiteral(" %"));
    bgOpacityRow->addWidget(_bgOpacitySpin);
    bgOpacityRow->addStretch();
    apForm->addRow(i18n("Background image opacity:"), bgOpacityRow);

    _bgStretchCombo = new QComboBox;
    _bgStretchCombo->addItem(i18n("None"),             QStringLiteral("none"));
    _bgStretchCombo->addItem(i18n("Fill"),             QStringLiteral("fill"));
    _bgStretchCombo->addItem(i18n("Uniform (fit)"),    QStringLiteral("uniform"));
    _bgStretchCombo->addItem(i18n("Uniform to fill"),  QStringLiteral("uniformToFill"));
    apForm->addRow(i18n("Background image stretch:"), _bgStretchCombo);

    _bgAlignCombo = new QComboBox;
    _bgAlignCombo->addItem(i18n("Top left"),      QStringLiteral("topLeft"));
    _bgAlignCombo->addItem(i18n("Top center"),    QStringLiteral("topCenter"));
    _bgAlignCombo->addItem(i18n("Top right"),     QStringLiteral("topRight"));
    _bgAlignCombo->addItem(i18n("Middle left"),   QStringLiteral("middleLeft"));
    _bgAlignCombo->addItem(i18n("Center"),        QStringLiteral("middleCenter"));
    _bgAlignCombo->addItem(i18n("Middle right"),  QStringLiteral("middleRight"));
    _bgAlignCombo->addItem(i18n("Bottom left"),   QStringLiteral("bottomLeft"));
    _bgAlignCombo->addItem(i18n("Bottom center"), QStringLiteral("bottomCenter"));
    _bgAlignCombo->addItem(i18n("Bottom right"),  QStringLiteral("bottomRight"));
    apForm->addRow(i18n("Background image alignment:"), _bgAlignCombo);

    contentLayout->addWidget(appearGroup);

    // ── Advanced group ────────────────────────────────────────────────────────
    auto* advGroup = new QGroupBox(i18n("Advanced"));
    auto* advForm = new QFormLayout(advGroup);
    advForm->setSpacing(10);

    auto* histRow = new QHBoxLayout;
    _historySizeSpin = new QSpinBox;
    _historySizeSpin->setRange(0, 100000);
    histRow->addWidget(_historySizeSpin);
    histRow->addStretch();
    advForm->addRow(i18n("Scrollback lines:"), histRow);

    _scrollbarStateCombo = new QComboBox;
    _scrollbarStateCombo->addItem(i18n("Visible"),  QStringLiteral("visible"));
    _scrollbarStateCombo->addItem(i18n("Hidden"),   QStringLiteral("hidden"));
    _scrollbarStateCombo->addItem(i18n("Always on"),QStringLiteral("always"));
    advForm->addRow(i18n("Scrollbar:"), _scrollbarStateCombo);

    auto* paddingRow = new QHBoxLayout;
    _paddingSpin = new QSpinBox;
    _paddingSpin->setRange(0, 64);
    _paddingSpin->setSuffix(QStringLiteral(" px"));
    paddingRow->addWidget(_paddingSpin);
    paddingRow->addStretch();
    advForm->addRow(i18n("Padding:"), paddingRow);

    _snapOnInputCheck  = new QCheckBox(i18n("Scroll to bottom on input"));
    _altGrAliasingCheck= new QCheckBox(i18n("AltGr aliasing (treat Right Alt as AltGr)"));
    advForm->addRow(QString{}, _snapOnInputCheck);
    advForm->addRow(QString{}, _altGrAliasingCheck);

    // Environment variables table
    auto* envLabel = new QLabel(i18n("Environment variables:"));
    advForm->addRow(envLabel);

    _envVarsTable = new QTableWidget(0, 2);
    _envVarsTable->setHorizontalHeaderLabels({i18n("Variable"), i18n("Value")});
    _envVarsTable->horizontalHeader()->setStretchLastSection(true);
    _envVarsTable->verticalHeader()->setVisible(false);
    _envVarsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    _envVarsTable->setMinimumHeight(120);
    advForm->addRow(_envVarsTable);

    auto* envBtnRow = new QHBoxLayout;
    _addEnvVarBtn    = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")),    i18n("Add"));
    _removeEnvVarBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), i18n("Remove"));
    envBtnRow->addWidget(_addEnvVarBtn);
    envBtnRow->addWidget(_removeEnvVarBtn);
    envBtnRow->addStretch();
    advForm->addRow(envBtnRow);

    contentLayout->addWidget(advGroup);
    contentLayout->addStretch();

    scroll->setWidget(content);
    outerLayout->addWidget(scroll, 1);

    _profilePageIdx = _stack->count();
    _stack->addWidget(outerPage); // index 3

    // ── Connect browse buttons ────────────────────────────────────────────────
    connect(browseShellBtn, &QPushButton::clicked, this, &SettingsDialog::_browseShell);
    connect(browseWdBtn,    &QPushButton::clicked, this, &SettingsDialog::_browseWorkDir);
    connect(browseBgBtn,    &QPushButton::clicked, this, &SettingsDialog::_browseBgImage);
    connect(clearBgBtn,     &QPushButton::clicked, this, [this]() { _bgImageEdit->clear(); });
    connect(_deleteProfileBtn, &QPushButton::clicked, this, [this]() {
        _removeProfile(_editingProfileKey);
    });
    connect(_addEnvVarBtn, &QPushButton::clicked, this, [this]() {
        const int row = _envVarsTable->rowCount();
        _envVarsTable->insertRow(row);
        _envVarsTable->setItem(row, 0, new QTableWidgetItem);
        _envVarsTable->setItem(row, 1, new QTableWidgetItem);
        _envVarsTable->setCurrentCell(row, 0);
        _envVarsTable->editItem(_envVarsTable->item(row, 0));
    });
    connect(_removeEnvVarBtn, &QPushButton::clicked, this, [this]() {
        const auto sel = _envVarsTable->selectedItems();
        if (!sel.isEmpty()) {
            _envVarsTable->removeRow(sel.first()->row());
        }
    });
}

void SettingsDialog::_buildColorSchemesPage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);
    vbox->addWidget(makeSectionTitle(i18n("Color Schemes")));

    auto* hbox = new QHBoxLayout;

    // Scheme list
    auto* schemeList = new QListWidget;
    schemeList->setMaximumWidth(200);
    const auto& schemes = LTermSettings::instance().colorSchemes();
    for (const QString& name : schemes.keys()) {
        schemeList->addItem(name);
    }
    hbox->addWidget(schemeList);

    // Preview panel
    auto* previewWidget = new QWidget;
    auto* previewLayout = new QVBoxLayout(previewWidget);
    previewLayout->setContentsMargins(16, 0, 0, 0);
    auto* previewLabel = new QLabel(i18n("Select a scheme to preview"));
    previewLabel->setAlignment(Qt::AlignTop);
    previewLayout->addWidget(previewLabel);

    auto* swatchArea = new QWidget;
    swatchArea->setMinimumHeight(80);
    swatchArea->setVisible(false);
    previewLayout->addWidget(swatchArea);
    previewLayout->addStretch();
    hbox->addWidget(previewWidget, 1);

    connect(schemeList, &QListWidget::currentTextChanged, this,
        [previewLabel, swatchArea, &schemes](const QString& name) {
            if (!schemes.contains(name)) {
                previewLabel->setText(i18n("Select a scheme to preview"));
                swatchArea->setVisible(false);
                return;
            }
            const ColorScheme& cs = schemes[name];
            // Build a color swatch row as HTML
            QString html = QStringLiteral("<b>%1</b><br>").arg(name.toHtmlEscaped());
            html += QStringLiteral("FG: <span style='color:%1'>%1</span> &nbsp; "
                                   "BG: <span style='color:%2; background:%2'>%2</span><br>")
                    .arg(cs.foreground.name(), cs.background.name());
            html += QStringLiteral("<br>ANSI colors:<br>");
            for (int i = 0; i < 16; ++i) {
                const QColor& c = cs.ansiColors[i];
                html += QStringLiteral(
                    "<span style='background:%1; color:%2; padding:2px 6px; margin:1px;'>%3</span>")
                        .arg(c.name(),
                             c.lightness() > 128 ? QStringLiteral("black") : QStringLiteral("white"),
                             QString::number(i));
            }
            previewLabel->setText(html);
            previewLabel->setVisible(true);
        });

    vbox->addLayout(hbox, 1);
    _stack->addWidget(page); // index 4
}

// ── Nav item click handling ────────────────────────────────────────────────────

void SettingsDialog::_onNavItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    const int type = item->data(NavTypeRole).toInt();
    if (type == NavHeader) return;

    // Always save the profile form if a profile was being edited
    _saveCurrentFormToProfile();

    switch (type) {
    case NavStartup:
        _stack->setCurrentIndex(0);
        break;
    case NavInteraction:
        _stack->setCurrentIndex(1);
        break;
    case NavAppearanceGlobal:
        _stack->setCurrentIndex(2);
        break;
    case NavProfile: {
        const QString name = item->data(NavDataRole).toString();
        _editingProfileKey = name;
        _profileTitle->setText(name);
        if (_localProfiles.contains(name)) {
            _loadProfileToForm(_localProfiles[name]);
        }
        _deleteProfileBtn->setEnabled(_localProfiles.size() > 1);
        _stack->setCurrentIndex(_profilePageIdx);
        break;
    }
    case NavAddProfile:
        _addProfile();
        break;
    case NavColorSchemes:
        _stack->setCurrentIndex(4);
        break;
    default:
        break;
    }
}

// ── Profile add / remove ──────────────────────────────────────────────────────

void SettingsDialog::_addProfile()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this, i18n("New Profile"),
        i18n("Enter a name for the new profile:"),
        QLineEdit::Normal, i18n("New Profile"), &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    const QString trimmed = name.trimmed();
    if (_localProfiles.contains(trimmed)) {
        QMessageBox::warning(this, i18n("Duplicate Profile"),
            i18n("A profile named '%1' already exists.").arg(trimmed));
        return;
    }

    Profile p;
    p.name = trimmed;
    _localProfiles[trimmed] = p;

    _syncDefaultCombo();
    _rebuildProfileNavItems();

    // Select the new profile in the nav
    for (int i = 0; i < _nav->count(); ++i) {
        auto* navItem = _nav->item(i);
        if (navItem->data(NavTypeRole).toInt() == NavProfile &&
            navItem->data(NavDataRole).toString() == trimmed) {
            _nav->setCurrentItem(navItem);
            _onNavItemClicked(navItem);
            break;
        }
    }
}

void SettingsDialog::_removeProfile(const QString& profileName)
{
    if (_localProfiles.size() <= 1) {
        QMessageBox::information(this, i18n("Cannot Remove"),
            i18n("At least one profile must exist."));
        return;
    }

    const int ret = QMessageBox::question(this,
        i18n("Remove Profile"),
        i18n("Remove profile '%1'? This cannot be undone.").arg(profileName),
        QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) return;

    _localProfiles.remove(profileName);
    if (_localGlobal.defaultProfileName == profileName) {
        _localGlobal.defaultProfileName = _localProfiles.firstKey();
    }

    _editingProfileKey.clear();
    _syncDefaultCombo();
    _rebuildProfileNavItems();
    _stack->setCurrentIndex(0);
}

// ── Browse dialogs ────────────────────────────────────────────────────────────

void SettingsDialog::_browseShell()
{
    const QString path = QFileDialog::getOpenFileName(
        this, i18n("Select Shell"), QStringLiteral("/bin"));
    if (!path.isEmpty()) _shellEdit->setText(path);
}

void SettingsDialog::_browseWorkDir()
{
    const QString start = _workDirEdit->text().isEmpty()
        ? QDir::homePath() : _workDirEdit->text();
    const QString path = QFileDialog::getExistingDirectory(
        this, i18n("Select Starting Directory"), start);
    if (!path.isEmpty()) _workDirEdit->setText(path);
}

void SettingsDialog::_browseBgImage()
{
    const QString start = _bgImageEdit->text().isEmpty()
        ? QDir::homePath() : _bgImageEdit->text();
    const QString path = QFileDialog::getOpenFileName(
        this, i18n("Select Background Image"), start,
        i18n("Images (*.png *.jpg *.jpeg *.bmp *.gif *.webp);;All Files (*)"));
    if (!path.isEmpty()) _bgImageEdit->setText(path);
}

// ── Profile form save / load ──────────────────────────────────────────────────

void SettingsDialog::_saveCurrentFormToProfile()
{
    if (_editingProfileKey.isEmpty() ||
        !_localProfiles.contains(_editingProfileKey)) return;

    Profile& p = _localProfiles[_editingProfileKey];

    // General
    const QString newName = _nameEdit->text().trimmed();
    if (!newName.isEmpty() && newName != _editingProfileKey) {
        // Rename: re-insert with new key
        _localProfiles.remove(_editingProfileKey);
        p.name = newName;
        _localProfiles[newName] = p;
        if (_localGlobal.defaultProfileName == _editingProfileKey) {
            _localGlobal.defaultProfileName = newName;
        }
        _editingProfileKey = newName;
    }
    p.name               = _editingProfileKey;
    p.shell              = _shellEdit->text().trimmed();
    p.workingDirectory   = _workDirEdit->text().trimmed();
    p.tabTitle           = _tabTitleEdit->text().trimmed();
    p.tabColor           = _tabColor.has_value() ? _tabColor->name(QColor::HexRgb) : QString{};
    p.hidden             = _hiddenCheck->isChecked();
    p.suppressApplicationTitle = _suppressTitleCheck->isChecked();
    p.closeOnExit        = static_cast<CloseOnExit>(_closeOnExitCombo->currentIndex());

    // Font
    p.fontFamily         = _fontCombo->currentFont().family();
    p.fontSize           = _fontSizeSpin->value();
    p.fontWeight         = static_cast<FontWeight>(_fontWeightCombo->currentIndex());
    p.enableBuiltinGlyphs= _enableBuiltinGlyphsCheck->isChecked();
    p.enableColorGlyphs  = _enableColorGlyphsCheck->isChecked();

    // Appearance
    p.colorScheme        = _schemeCombo->currentText();
    p.cursorShape        = static_cast<CursorShape>(_cursorShapeCombo->currentIndex());
    p.cursorHeight       = _cursorHeightSpin->value();
    p.cursorColor        = _cursorColor;
    p.foregroundColorOverride = _fgColor;
    p.backgroundColorOverride = _bgColor;
    p.selectionBackground     = _selBgColor;

    // Background image
    p.backgroundImagePath       = _bgImageEdit->text().trimmed();
    p.backgroundImageOpacity    = _bgOpacitySpin->value() / 100.0;
    p.backgroundImageStretchMode= static_cast<BgStretchMode>(_bgStretchCombo->currentIndex());
    p.backgroundImageAlignment  = static_cast<BgAlignment>(_bgAlignCombo->currentIndex());

    // Advanced
    p.historySize        = _historySizeSpin->value();
    p.scrollbarState     = static_cast<ScrollbarState>(_scrollbarStateCombo->currentIndex());
    p.padding            = _paddingSpin->value();
    p.snapOnInput        = _snapOnInputCheck->isChecked();
    p.altGrAliasing      = _altGrAliasingCheck->isChecked();

    p.environmentVariables.clear();
    for (int row = 0; row < _envVarsTable->rowCount(); ++row) {
        auto* kItem = _envVarsTable->item(row, 0);
        auto* vItem = _envVarsTable->item(row, 1);
        if (kItem && !kItem->text().trimmed().isEmpty()) {
            p.environmentVariables[kItem->text().trimmed()] =
                vItem ? vItem->text() : QString{};
        }
    }
}

void SettingsDialog::_loadProfileToForm(const Profile& p)
{
    // General
    _nameEdit->setText(p.name);
    _shellEdit->setText(p.shell);
    _workDirEdit->setText(p.workingDirectory);
    _tabTitleEdit->setText(p.tabTitle);
    _tabColor = p.tabColor.isEmpty() ? std::nullopt
                                     : std::optional<QColor>(QColor(p.tabColor));
    _updateColorBtn(_tabColorBtn, _tabColor);
    _hiddenCheck->setChecked(p.hidden);
    _suppressTitleCheck->setChecked(p.suppressApplicationTitle);
    _closeOnExitCombo->setCurrentIndex(static_cast<int>(p.closeOnExit));

    // Font
    _fontCombo->blockSignals(true);
    _fontCombo->setCurrentFont(QFont(p.fontFamily));
    _fontCombo->blockSignals(false);
    _fontSizeSpin->setValue(p.fontSize);
    _fontWeightCombo->setCurrentIndex(static_cast<int>(p.fontWeight));
    _enableBuiltinGlyphsCheck->setChecked(p.enableBuiltinGlyphs);
    _enableColorGlyphsCheck->setChecked(p.enableColorGlyphs);

    // Appearance
    const int schemeIdx = _schemeCombo->findText(p.colorScheme);
    _schemeCombo->setCurrentIndex(schemeIdx >= 0 ? schemeIdx : 0);
    _cursorShapeCombo->setCurrentIndex(static_cast<int>(p.cursorShape));
    _cursorHeightSpin->setValue(p.cursorHeight);
    _cursorColor = p.cursorColor;       _updateColorBtn(_cursorColorBtn, _cursorColor);
    _fgColor     = p.foregroundColorOverride; _updateColorBtn(_fgColorBtn, _fgColor);
    _bgColor     = p.backgroundColorOverride; _updateColorBtn(_bgColorBtn, _bgColor);
    _selBgColor  = p.selectionBackground;     _updateColorBtn(_selBgColorBtn, _selBgColor);

    // Background image
    _bgImageEdit->setText(p.backgroundImagePath);
    _bgOpacitySpin->setValue(qRound(p.backgroundImageOpacity * 100.0));
    _bgStretchCombo->setCurrentIndex(static_cast<int>(p.backgroundImageStretchMode));
    _bgAlignCombo->setCurrentIndex(static_cast<int>(p.backgroundImageAlignment));

    // Advanced
    _historySizeSpin->setValue(p.historySize);
    _scrollbarStateCombo->setCurrentIndex(static_cast<int>(p.scrollbarState));
    _paddingSpin->setValue(p.padding);
    _snapOnInputCheck->setChecked(p.snapOnInput);
    _altGrAliasingCheck->setChecked(p.altGrAliasing);

    _envVarsTable->setRowCount(0);
    int envRow = 0;
    for (auto it = p.environmentVariables.constBegin();
         it != p.environmentVariables.constEnd(); ++it, ++envRow) {
        _envVarsTable->insertRow(envRow);
        _envVarsTable->setItem(envRow, 0, new QTableWidgetItem(it.key()));
        _envVarsTable->setItem(envRow, 1, new QTableWidgetItem(it.value()));
    }
}

// ── Global settings load / save ───────────────────────────────────────────────

void SettingsDialog::_loadGlobalToStartup()
{
    _syncDefaultCombo();

    const int lmIdx = _launchModeCombo->findData([]() -> QString {
        switch (LTermSettings::instance().global().launchMode) {
        case LaunchMode::Maximized:  return QStringLiteral("maximized");
        case LaunchMode::Fullscreen: return QStringLiteral("fullscreen");
        case LaunchMode::Focus:      return QStringLiteral("focus");
        default:                     return QStringLiteral("default");
        }
    }());
    _launchModeCombo->setCurrentIndex(lmIdx >= 0 ? lmIdx : 0);

    _initialRowsSpin->setValue(_localGlobal.initialRows);
    _initialColsSpin->setValue(_localGlobal.initialCols);

    const int twIdx = _tabWidthModeCombo->findData([]() -> QString {
        switch (LTermSettings::instance().global().tabWidthMode) {
        case TabWidthMode::Compact:     return QStringLiteral("compact");
        case TabWidthMode::TitleLength: return QStringLiteral("titleLength");
        default:                        return QStringLiteral("equal");
        }
    }());
    _tabWidthModeCombo->setCurrentIndex(twIdx >= 0 ? twIdx : 0);

    _centerOnLaunchCheck->setChecked(_localGlobal.centerOnLaunch);
    _alwaysOnTopCheck->setChecked(_localGlobal.alwaysOnTop);
    _alwaysShowTabsCheck->setChecked(_localGlobal.alwaysShowTabs);
}

void SettingsDialog::_loadGlobalToInteraction()
{
    _copyOnSelectCheck->setChecked(_localGlobal.copyOnSelect);
    _trimPasteCheck->setChecked(_localGlobal.trimPaste);
    _wordDelimitersEdit->setText(_localGlobal.wordDelimiters);
}

void SettingsDialog::_loadGlobalToAppearance()
{
    const QString themeKey = [this]() -> QString {
        switch (_localGlobal.theme) {
        case AppTheme::Dark:  return QStringLiteral("dark");
        case AppTheme::Light: return QStringLiteral("light");
        default:              return QStringLiteral("system");
        }
    }();
    const int idx = _themeCombo->findData(themeKey);
    _themeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void SettingsDialog::_saveGlobalForms()
{
    // Startup
    _localGlobal.defaultProfileName = _defaultCombo->currentText();

    const QString lmStr = _launchModeCombo->currentData().toString();
    if (lmStr == QStringLiteral("maximized"))  _localGlobal.launchMode = LaunchMode::Maximized;
    else if (lmStr == QStringLiteral("fullscreen")) _localGlobal.launchMode = LaunchMode::Fullscreen;
    else if (lmStr == QStringLiteral("focus")) _localGlobal.launchMode = LaunchMode::Focus;
    else                                        _localGlobal.launchMode = LaunchMode::Default;

    _localGlobal.initialRows    = _initialRowsSpin->value();
    _localGlobal.initialCols    = _initialColsSpin->value();
    _localGlobal.centerOnLaunch = _centerOnLaunchCheck->isChecked();
    _localGlobal.alwaysOnTop    = _alwaysOnTopCheck->isChecked();
    _localGlobal.alwaysShowTabs = _alwaysShowTabsCheck->isChecked();

    const QString twStr = _tabWidthModeCombo->currentData().toString();
    if (twStr == QStringLiteral("compact"))       _localGlobal.tabWidthMode = TabWidthMode::Compact;
    else if (twStr == QStringLiteral("titleLength")) _localGlobal.tabWidthMode = TabWidthMode::TitleLength;
    else                                           _localGlobal.tabWidthMode = TabWidthMode::Equal;

    // Interaction
    _localGlobal.copyOnSelect    = _copyOnSelectCheck->isChecked();
    _localGlobal.trimPaste       = _trimPasteCheck->isChecked();
    _localGlobal.wordDelimiters  = _wordDelimitersEdit->text();

    // Appearance
    const QString themeStr = _themeCombo->currentData().toString();
    if (themeStr == QStringLiteral("dark"))       _localGlobal.theme = AppTheme::Dark;
    else if (themeStr == QStringLiteral("light")) _localGlobal.theme = AppTheme::Light;
    else                                          _localGlobal.theme = AppTheme::System;
}

void SettingsDialog::_syncDefaultCombo()
{
    if (!_defaultCombo) return;
    const QString current = _defaultCombo->currentText().isEmpty()
        ? _localGlobal.defaultProfileName
        : _defaultCombo->currentText();
    _defaultCombo->clear();
    for (const QString& name : _localProfiles.keys()) {
        _defaultCombo->addItem(name);
    }
    const int idx = _defaultCombo->findText(current);
    _defaultCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

// ── Save ──────────────────────────────────────────────────────────────────────

void SettingsDialog::_save()
{
    _saveCurrentFormToProfile();
    _saveGlobalForms();

    auto& s = LTermSettings::instance();

    // Remove profiles deleted in the dialog
    const QList<QString> existingKeys = s.profiles().keys();
    for (const QString& name : existingKeys) {
        if (!_localProfiles.contains(name)) {
            s.removeProfile(name);
        }
    }

    // Apply all local profiles
    for (const Profile& p : std::as_const(_localProfiles)) {
        s.setProfile(p);
    }

    // Apply global settings
    s.setGlobal(_localGlobal);

    s.save();
    accept();
}

} // namespace LTerm
