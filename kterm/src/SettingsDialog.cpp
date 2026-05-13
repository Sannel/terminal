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
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpacerItem>
#include <QSpinBox>
#include <QSizePolicy>
#include <QTabWidget>
#include <QVBoxLayout>

namespace KTerm {

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(i18n("KTerm Settings"));
    setMinimumSize(640, 460);

    auto& s = KTermSettings::instance();
    _localProfiles = s.profiles();

    auto* outer = new QVBoxLayout(this);
    auto* tabs  = new QTabWidget(this);

    // ── General tab ────────────────────────────────────────────────────────────
    {
        auto* w    = new QWidget;
        auto* form = new QFormLayout(w);
        form->setContentsMargins(12, 12, 12, 12);
        form->setSpacing(10);

        _defaultCombo = new QComboBox(w);
        _syncDefaultCombo();
        _defaultCombo->setCurrentText(s.defaultProfileName());
        form->addRow(i18n("Default Profile:"), _defaultCombo);

        // Push form to top
        auto* spacer = new QWidget(w);
        spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        form->addRow(spacer);

        tabs->addTab(w, i18n("General"));
    }

    // ── Profiles tab ───────────────────────────────────────────────────────────
    {
        auto* w    = new QWidget;
        auto* hbox = new QHBoxLayout(w);
        hbox->setContentsMargins(8, 8, 8, 8);
        hbox->setSpacing(8);

        // Left: list + Add/Remove buttons
        auto* leftBox  = new QVBoxLayout;
        _profileList   = new QListWidget(w);
        _profileList->setMaximumWidth(180);
        for (const QString& name : _localProfiles.keys()) {
            _profileList->addItem(name);
        }
        leftBox->addWidget(_profileList);

        auto* listBtns = new QHBoxLayout;
        auto* addBtn   = new QPushButton(QIcon::fromTheme(QStringLiteral("list-add")), QString{}, w);
        addBtn->setToolTip(i18n("Add Profile"));
        addBtn->setFixedWidth(32);
        _removeBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("list-remove")), QString{}, w);
        _removeBtn->setToolTip(i18n("Remove Profile"));
        _removeBtn->setFixedWidth(32);
        _removeBtn->setEnabled(false);
        listBtns->addWidget(addBtn);
        listBtns->addWidget(_removeBtn);
        listBtns->addStretch();
        leftBox->addLayout(listBtns);
        hbox->addLayout(leftBox);

        // Right: form group box
        _formGroup = new QGroupBox(i18n("Profile Settings"), w);
        auto* form = new QFormLayout(_formGroup);
        form->setContentsMargins(10, 16, 10, 10);
        form->setSpacing(8);

        // Shell
        auto* shellRow = new QHBoxLayout;
        shellRow->setSpacing(4);
        _shellEdit = new QLineEdit(_formGroup);
        _shellEdit->setPlaceholderText(i18n("(default: $SHELL)"));
        auto* shellBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), QString{}, _formGroup);
        shellBtn->setFixedWidth(32);
        shellBtn->setToolTip(i18n("Browse…"));
        shellRow->addWidget(_shellEdit);
        shellRow->addWidget(shellBtn);
        form->addRow(i18n("Shell:"), shellRow);

        // Working directory
        auto* wdRow = new QHBoxLayout;
        wdRow->setSpacing(4);
        _workDirEdit = new QLineEdit(_formGroup);
        _workDirEdit->setPlaceholderText(i18n("(default: home directory)"));
        auto* wdBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-open")), QString{}, _formGroup);
        wdBtn->setFixedWidth(32);
        wdBtn->setToolTip(i18n("Browse…"));
        wdRow->addWidget(_workDirEdit);
        wdRow->addWidget(wdBtn);
        form->addRow(i18n("Working Directory:"), wdRow);

        // Font
        auto* fontRow = new QHBoxLayout;
        fontRow->setSpacing(4);
        _fontCombo = new QFontComboBox(_formGroup);
        _fontCombo->setFontFilters(QFontComboBox::MonospacedFonts);
        _fontSizeSpin = new QSpinBox(_formGroup);
        _fontSizeSpin->setRange(6, 72);
        _fontSizeSpin->setValue(11);
        _fontSizeSpin->setSuffix(QStringLiteral(" pt"));
        fontRow->addWidget(_fontCombo, 3);
        fontRow->addWidget(_fontSizeSpin, 1);
        form->addRow(i18n("Font:"), fontRow);

        // Color scheme
        _schemeCombo = new QComboBox(_formGroup);
        for (const QString& name : s.colorSchemes().keys()) {
            _schemeCombo->addItem(name);
        }
        form->addRow(i18n("Color Scheme:"), _schemeCombo);

        _formGroup->setEnabled(false);
        hbox->addWidget(_formGroup, 1);

        tabs->addTab(w, i18n("Profiles"));

        // Connections
        connect(_profileList, &QListWidget::currentRowChanged,
                this, &SettingsDialog::_onProfileSelected);
        connect(addBtn,     &QPushButton::clicked, this, &SettingsDialog::_addProfile);
        connect(_removeBtn, &QPushButton::clicked, this, &SettingsDialog::_removeProfile);
        connect(shellBtn,   &QPushButton::clicked, this, &SettingsDialog::_browseShell);
        connect(wdBtn,      &QPushButton::clicked, this, &SettingsDialog::_browseWorkDir);

        if (_profileList->count() > 0) {
            _profileList->setCurrentRow(0);
        }
    }

    outer->addWidget(tabs);

    auto* btnBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, this);
    connect(btnBox, &QDialogButtonBox::accepted, this, [this]() { _apply(); accept(); });
    connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(btnBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &SettingsDialog::_apply);
    outer->addWidget(btnBox);
}

// ── Profile list ─────────────────────────────────────────────────────────────

void SettingsDialog::_onProfileSelected(int row)
{
    _saveCurrentFormToProfile();

    if (row < 0 || row >= _profileList->count()) {
        _formGroup->setEnabled(false);
        _formGroup->setTitle(i18n("Profile Settings"));
        _removeBtn->setEnabled(false);
        _editingKey.clear();
        return;
    }

    const QString name = _profileList->item(row)->text();
    _editingKey = name;
    _loadProfileToForm(_localProfiles.value(name));
    _formGroup->setTitle(name);
    _formGroup->setEnabled(true);
    _removeBtn->setEnabled(true);
}

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

    _profileList->addItem(name);
    _syncDefaultCombo();
    _profileList->setCurrentRow(_profileList->count() - 1);
}

void SettingsDialog::_removeProfile()
{
    const int row = _profileList->currentRow();
    if (row < 0) return;

    if (_localProfiles.size() <= 1) {
        QMessageBox::information(this, i18n("Cannot Remove"),
            i18n("At least one profile must remain."));
        return;
    }

    const QString name = _profileList->item(row)->text();
    if (QMessageBox::question(this, i18n("Remove Profile"),
            i18n("Remove profile \"%1\"?").arg(name),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    _editingKey.clear();
    _localProfiles.remove(name);
    delete _profileList->takeItem(row);
    _syncDefaultCombo();

    const int newRow = qMin(row, _profileList->count() - 1);
    if (newRow >= 0) {
        _profileList->setCurrentRow(newRow);
    } else {
        _formGroup->setEnabled(false);
        _formGroup->setTitle(i18n("Profile Settings"));
        _removeBtn->setEnabled(false);
    }
}

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
        this, i18n("Select Working Directory"), start);
    if (!path.isEmpty()) _workDirEdit->setText(path);
}

// ── Form helpers ──────────────────────────────────────────────────────────────

void SettingsDialog::_saveCurrentFormToProfile()
{
    if (_editingKey.isEmpty() || !_localProfiles.contains(_editingKey)) return;

    Profile& p         = _localProfiles[_editingKey];
    p.shell            = _shellEdit->text().trimmed();
    p.workingDirectory = _workDirEdit->text().trimmed();
    p.fontFamily       = _fontCombo->currentFont().family();
    p.fontSize         = _fontSizeSpin->value();
    p.colorScheme      = _schemeCombo->currentText();
}

void SettingsDialog::_loadProfileToForm(const Profile& p)
{
    _shellEdit->setText(p.shell);
    _workDirEdit->setText(p.workingDirectory);

    // Preserve the saved font even if QFontComboBox filter doesn't show it.
    _fontCombo->blockSignals(true);
    _fontCombo->setCurrentFont(QFont(p.fontFamily));
    _fontCombo->blockSignals(false);

    _fontSizeSpin->setValue(p.fontSize);

    const int idx = _schemeCombo->findText(p.colorScheme);
    _schemeCombo->setCurrentIndex(idx >= 0 ? idx : 0);
}

void SettingsDialog::_syncDefaultCombo()
{
    const QString current = _defaultCombo ? _defaultCombo->currentText() : QString{};
    if (_defaultCombo) {
        _defaultCombo->clear();
        for (const QString& name : _localProfiles.keys()) {
            _defaultCombo->addItem(name);
        }
        const int idx = _defaultCombo->findText(current);
        _defaultCombo->setCurrentIndex(idx >= 0 ? idx : 0);
    }
}

// ── Apply ─────────────────────────────────────────────────────────────────────

void SettingsDialog::_apply()
{
    _saveCurrentFormToProfile();

    auto& s = KTermSettings::instance();

    // Remove profiles deleted during this dialog session.
    for (const QString& name : s.profiles().keys()) {
        if (!_localProfiles.contains(name)) {
            s.removeProfile(name);
        }
    }

    // Add / update all remaining profiles.
    for (const Profile& p : std::as_const(_localProfiles)) {
        s.setProfile(p);
    }

    s.setDefaultProfileName(_defaultCombo->currentText());
    s.save();
}

} // namespace KTerm
