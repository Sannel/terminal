// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "SettingsDialog.hpp"

#include <KLocalizedString>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFontComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
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
#include <QVBoxLayout>
#include <QFrame>

namespace LTerm {

// ── Data roles stored in nav list items ──────────────────────────────────────
static constexpr int NavTypeRole = Qt::UserRole;
static constexpr int NavDataRole = Qt::UserRole + 1;

enum NavItemType {
    NavHeader   = 0,  // section header — not selectable
    NavStartup  = 1,
    NavProfile  = 2,
    NavAddProfile = 3,
    NavColorSchemes = 4,
};

// ── Constructor ───────────────────────────────────────────────────────────────

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(i18n("LTerm Settings"));
    setMinimumSize(820, 580);
    resize(820, 580);

    auto& s = LTermSettings::instance();
    _localProfiles = s.profiles();

    // ── Top-level layout: nav | stack ────────────────────────────────────────
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Left sidebar ─────────────────────────────────────────────────────────
    auto* sidePanel = new QWidget(this);
    sidePanel->setFixedWidth(210);
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
    _buildStartupPage();
    _buildProfilePage(QString{});  // shared profile form page
    _buildColorSchemesPage();

    // ── Connections ───────────────────────────────────────────────────────────
    connect(_nav, &QListWidget::itemClicked, this, &SettingsDialog::_onNavItemClicked);
    connect(_saveBtn,   &QPushButton::clicked, this, &SettingsDialog::_save);
    connect(cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);

    // Select "Startup" on open
    for (int i = 0; i < _nav->count(); ++i) {
        auto* item = _nav->item(i);
        if (item->data(NavTypeRole).toInt() == NavStartup) {
            _nav->setCurrentItem(item);
            _stack->setCurrentIndex(0); // startup page is always index 0
            break;
        }
    }
}

// ── Nav building ─────────────────────────────────────────────────────────────

QListWidgetItem* SettingsDialog::_addNavHeader(const QString& text)
{
    auto* item = new QListWidgetItem(text.toUpper(), _nav);
    item->setData(NavTypeRole, NavHeader);
    item->setFlags(Qt::NoItemFlags); // not selectable
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
    auto* item = new QListWidgetItem(icon, text, _nav);
    item->setData(NavDataRole, data);
    if (indent > 0) {
        // Visual indent via left-padding in the text
        item->setText(QStringLiteral("   ") + text);
    }
    return item;
}

void SettingsDialog::_buildNav()
{
    _nav->clear();

    // Startup
    auto* startupItem = _addNavItem(
        i18n("Startup"),
        QIcon::fromTheme(QStringLiteral("preferences-system")),
        QStringLiteral("startup"));
    startupItem->setData(NavTypeRole, NavStartup);

    // Profiles section
    _addNavHeader(i18n("Profiles"));
    _rebuildProfileNavItems();

    // Color Schemes
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

    // Find insertion point: after the "Profiles" header
    int insertAt = 0;
    for (int i = 0; i < _nav->count(); ++i) {
        if (_nav->item(i)->data(NavTypeRole).toInt() == NavHeader) {
            // Check if this header says "Profiles" (check by position heuristic)
            insertAt = i + 1;
        }
    }

    // Insert before Color Schemes (find it)
    int csIdx = _nav->count();
    for (int i = 0; i < _nav->count(); ++i) {
        if (_nav->item(i)->data(NavTypeRole).toInt() == NavColorSchemes) {
            csIdx = i;
            break;
        }
    }

    int pos = (insertAt > 0) ? insertAt : csIdx;

    // Add one item per profile
    for (const QString& name : _localProfiles.keys()) {
        auto* item = new QListWidgetItem(
            QIcon::fromTheme(QStringLiteral("utilities-terminal")),
            QStringLiteral("   ") + name);
        item->setData(NavTypeRole, NavProfile);
        item->setData(NavDataRole, name);
        _nav->insertItem(pos++, item);
    }

    // "+ Add a new profile"
    auto* addItem = new QListWidgetItem(
        QIcon::fromTheme(QStringLiteral("list-add")),
        QStringLiteral("   ") + i18n("Add a new profile"));
    addItem->setData(NavTypeRole, NavAddProfile);
    _nav->insertItem(pos, addItem);
}

// ── Page building ─────────────────────────────────────────────────────────────

void SettingsDialog::_buildStartupPage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);

    // Page title
    auto* title = new QLabel(i18n("Startup"), page);
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.5);
    title->setFont(tf);
    vbox->addWidget(title);

    auto* sep = new QFrame(page);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(sep);

    // Default profile
    auto* form = new QFormLayout;
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    _defaultCombo = new QComboBox(page);
    _syncDefaultCombo();
    _defaultCombo->setCurrentText(LTermSettings::instance().defaultProfileName());
    _defaultCombo->setMinimumWidth(200);
    form->addRow(i18n("Default profile:"), _defaultCombo);

    vbox->addLayout(form);
    vbox->addStretch();

    _stack->addWidget(page); // index 0
}

void SettingsDialog::_buildProfilePage(const QString& /*profileName*/)
{
    // One shared profile page; content loaded dynamically when a profile is selected.
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);

    // Title (updated when profile changes)
    _profileTitle = new QLabel(page);
    QFont tf = _profileTitle->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.5);
    _profileTitle->setFont(tf);
    vbox->addWidget(_profileTitle);

    auto* sep = new QFrame(page);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(sep);

    // Scrollable form area
    auto* scrollArea = new QScrollArea(page);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidgetResizable(true);
    auto* formW = new QWidget;
    auto* form = new QFormLayout(formW);
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    form->setContentsMargins(0, 0, 0, 0);

    // Shell
    auto* shellRow = new QHBoxLayout;
    shellRow->setSpacing(4);
    _shellEdit = new QLineEdit(formW);
    _shellEdit->setPlaceholderText(i18n("Default: $SHELL"));
    auto* shellBtn = new QPushButton(
        QIcon::fromTheme(QStringLiteral("document-open")), QString{}, formW);
    shellBtn->setFixedWidth(32);
    shellRow->addWidget(_shellEdit);
    shellRow->addWidget(shellBtn);
    form->addRow(i18n("Shell executable:"), shellRow);

    // Working directory
    auto* wdRow = new QHBoxLayout;
    wdRow->setSpacing(4);
    _workDirEdit = new QLineEdit(formW);
    _workDirEdit->setPlaceholderText(i18n("Default: home directory"));
    auto* wdBtn = new QPushButton(
        QIcon::fromTheme(QStringLiteral("document-open")), QString{}, formW);
    wdBtn->setFixedWidth(32);
    wdRow->addWidget(_workDirEdit);
    wdRow->addWidget(wdBtn);
    form->addRow(i18n("Starting directory:"), wdRow);

    // Font
    auto* fontRow = new QHBoxLayout;
    fontRow->setSpacing(6);
    _fontCombo = new QFontComboBox(formW);
    _fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
    _fontSizeSpin = new QSpinBox(formW);
    _fontSizeSpin->setRange(6, 72);
    _fontSizeSpin->setValue(11);
    _fontSizeSpin->setSuffix(QStringLiteral(" pt"));
    _fontSizeSpin->setFixedWidth(80);
    fontRow->addWidget(_fontCombo, 1);
    fontRow->addWidget(_fontSizeSpin);
    form->addRow(i18n("Font:"), fontRow);

    // Color scheme
    _schemeCombo = new QComboBox(formW);
    for (const QString& name : LTermSettings::instance().colorSchemes().keys()) {
        _schemeCombo->addItem(name);
    }
    _schemeCombo->setMinimumWidth(200);
    form->addRow(i18n("Color scheme:"), _schemeCombo);

    // Background image
    auto* bgImageRow = new QHBoxLayout;
    bgImageRow->setSpacing(4);
    _bgImageEdit = new QLineEdit(formW);
    _bgImageEdit->setPlaceholderText(i18n("No background image"));
    auto* bgImageBtn = new QPushButton(
        QIcon::fromTheme(QStringLiteral("document-open")), QString{}, formW);
    bgImageBtn->setFixedWidth(32);
    bgImageBtn->setToolTip(i18n("Browse for image…"));
    auto* bgImageClear = new QPushButton(
        QIcon::fromTheme(QStringLiteral("edit-clear")), QString{}, formW);
    bgImageClear->setFixedWidth(32);
    bgImageClear->setToolTip(i18n("Clear background image"));
    bgImageRow->addWidget(_bgImageEdit);
    bgImageRow->addWidget(bgImageBtn);
    bgImageRow->addWidget(bgImageClear);
    form->addRow(i18n("Background image:"), bgImageRow);

    // Background opacity
    _bgOpacitySpin = new QSpinBox(formW);
    _bgOpacitySpin->setRange(0, 100);
    _bgOpacitySpin->setValue(50);
    _bgOpacitySpin->setSuffix(QStringLiteral(" %"));
    _bgOpacitySpin->setFixedWidth(90);
    _bgOpacitySpin->setToolTip(i18n("How visible the background image is (0% = hidden, 100% = fully opaque)"));
    form->addRow(i18n("Image opacity:"), _bgOpacitySpin);

    scrollArea->setWidget(formW);
    vbox->addWidget(scrollArea, 1);

    // Delete profile button (bottom of page, danger zone)
    auto* dangerSep = new QFrame(page);
    dangerSep->setFrameShape(QFrame::HLine);
    dangerSep->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(dangerSep);

    auto* dangerRow = new QHBoxLayout;
    dangerRow->setSpacing(8);
    dangerRow->addStretch();
    _deleteProfileBtn = new QPushButton(
        QIcon::fromTheme(QStringLiteral("edit-delete")),
        i18n("Delete this profile"), page);
    _deleteProfileBtn->setToolTip(i18n("Permanently remove this profile"));
    dangerRow->addWidget(_deleteProfileBtn);
    vbox->addLayout(dangerRow);

    // Connections
    connect(shellBtn,   &QPushButton::clicked, this, &SettingsDialog::_browseShell);
    connect(wdBtn,      &QPushButton::clicked, this, &SettingsDialog::_browseWorkDir);
    connect(bgImageBtn, &QPushButton::clicked, this, &SettingsDialog::_browseBgImage);
    connect(bgImageClear, &QPushButton::clicked, this, [this]() { _bgImageEdit->clear(); });
    connect(_deleteProfileBtn, &QPushButton::clicked, this, [this]() {
        _removeProfile(_editingProfileKey);
    });

    _profilePageIdx = _stack->addWidget(page);
}

void SettingsDialog::_buildColorSchemesPage()
{
    auto* page = new QWidget;
    auto* vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(32, 24, 32, 24);
    vbox->setSpacing(16);

    auto* title = new QLabel(i18n("Color Schemes"), page);
    QFont tf = title->font();
    tf.setPointSizeF(tf.pointSizeF() * 1.5);
    title->setFont(tf);
    vbox->addWidget(title);

    auto* sep = new QFrame(page);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    vbox->addWidget(sep);

    auto* info = new QLabel(
        i18n("Built-in color schemes are available to all profiles.\n"
             "Custom color scheme editing will be added in a future release."), page);
    info->setWordWrap(true);
    vbox->addWidget(info);

    // List of scheme names
    auto* list = new QListWidget(page);
    for (const QString& name : LTermSettings::instance().colorSchemes().keys()) {
        list->addItem(name);
    }
    list->setMaximumHeight(200);
    vbox->addWidget(list);
    vbox->addStretch();

    _stack->addWidget(page); // index 2
}

// ── Nav selection ─────────────────────────────────────────────────────────────

void SettingsDialog::_onNavItemClicked(QListWidgetItem* item)
{
    if (!item) return;
    const int type = item->data(NavTypeRole).toInt();

    if (type == NavHeader) {
        // Deselect — headers are not selectable
        _nav->clearSelection();
        return;
    }

    if (type == NavAddProfile) {
        _nav->clearSelection();
        _addProfile();
        return;
    }

    _selectNavItem(item);
}

void SettingsDialog::_selectNavItem(QListWidgetItem* item)
{
    _saveCurrentFormToProfile();

    const int type = item->data(NavTypeRole).toInt();

    if (type == NavStartup) {
        _stack->setCurrentIndex(0);
        return;
    }

    if (type == NavColorSchemes) {
        _stack->setCurrentIndex(2);
        return;
    }

    if (type == NavProfile) {
        const QString name = item->data(NavDataRole).toString();
        _editingProfileKey = name;
        _loadProfileToForm(_localProfiles.value(name));
        _profileTitle->setText(name);
        _deleteProfileBtn->setEnabled(_localProfiles.size() > 1);
        _stack->setCurrentIndex(_profilePageIdx);
    }
}

// ── Profile operations ────────────────────────────────────────────────────────

void SettingsDialog::_addProfile()
{
    _saveCurrentFormToProfile();

    bool ok = false;
    const QString raw = QInputDialog::getText(
        this, i18n("New Profile"), i18n("Profile name:"),
        QLineEdit::Normal, QString{}, &ok);
    if (!ok) return;
    const QString name = raw.trimmed();
    if (name.isEmpty()) return;

    if (_localProfiles.contains(name)) {
        QMessageBox::warning(this, i18n("Duplicate Name"),
            i18n("A profile named \"%1\" already exists.").arg(name));
        return;
    }

    Profile p;
    p.name = name;
    _localProfiles[name] = p;
    _syncDefaultCombo();
    _rebuildProfileNavItems();

    // Select the new profile in the nav
    for (int i = 0; i < _nav->count(); ++i) {
        auto* it = _nav->item(i);
        if (it->data(NavTypeRole).toInt() == NavProfile &&
            it->data(NavDataRole).toString() == name) {
            _nav->setCurrentItem(it);
            _selectNavItem(it);
            break;
        }
    }
}

void SettingsDialog::_removeProfile(const QString& profileName)
{
    if (_localProfiles.size() <= 1) {
        QMessageBox::information(this, i18n("Cannot Delete"),
            i18n("At least one profile must remain."));
        return;
    }
    if (QMessageBox::question(this, i18n("Delete Profile"),
            i18n("Delete profile \"%1\"? This cannot be undone.").arg(profileName),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    _editingProfileKey.clear();
    _localProfiles.remove(profileName);
    _syncDefaultCombo();
    _rebuildProfileNavItems();

    // Navigate to Startup page
    for (int i = 0; i < _nav->count(); ++i) {
        if (_nav->item(i)->data(NavTypeRole).toInt() == NavStartup) {
            _nav->setCurrentItem(_nav->item(i));
            _stack->setCurrentIndex(0);
            break;
        }
    }
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

// ── Form helpers ──────────────────────────────────────────────────────────────

void SettingsDialog::_saveCurrentFormToProfile()
{
    if (_editingProfileKey.isEmpty() ||
        !_localProfiles.contains(_editingProfileKey)) return;

    Profile& p            = _localProfiles[_editingProfileKey];
    p.shell               = _shellEdit->text().trimmed();
    p.workingDirectory    = _workDirEdit->text().trimmed();
    p.fontFamily          = _fontCombo->currentFont().family();
    p.fontSize            = _fontSizeSpin->value();
    p.colorScheme         = _schemeCombo->currentText();
    p.backgroundImagePath = _bgImageEdit->text().trimmed();
    p.backgroundOpacity   = _bgOpacitySpin->value() / 100.0;
}

void SettingsDialog::_loadProfileToForm(const Profile& p)
{
    _shellEdit->setText(p.shell);
    _workDirEdit->setText(p.workingDirectory);

    _fontCombo->blockSignals(true);
    _fontCombo->setCurrentFont(QFont(p.fontFamily));
    _fontCombo->blockSignals(false);

    _fontSizeSpin->setValue(p.fontSize);

    const int idx = _schemeCombo->findText(p.colorScheme);
    _schemeCombo->setCurrentIndex(idx >= 0 ? idx : 0);

    _bgImageEdit->setText(p.backgroundImagePath);
    _bgOpacitySpin->setValue(qRound(p.backgroundOpacity * 100.0));
}

void SettingsDialog::_syncDefaultCombo()
{
    if (!_defaultCombo) return;
    const QString current = _defaultCombo->currentText();
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

    auto& s = LTermSettings::instance();

    for (const QString& name : s.profiles().keys()) {
        if (!_localProfiles.contains(name)) s.removeProfile(name);
    }
    for (const Profile& p : std::as_const(_localProfiles)) {
        s.setProfile(p);
    }
    s.setDefaultProfileName(_defaultCombo->currentText());
    s.save();

    accept();
}

} // namespace LTerm

