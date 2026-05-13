// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "KTermSettings.hpp"

#include <QDialog>
#include <QMap>

class QComboBox;
class QFontComboBox;
class QGroupBox;
class QListWidget;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace KTerm {

/**
 * SettingsDialog — modal dialog for editing profiles and global settings.
 *
 * Maintains a local copy of profiles that is only pushed to KTermSettings
 * when the user clicks Apply or OK.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void _onProfileSelected(int row);
    void _addProfile();
    void _removeProfile();
    void _browseShell();
    void _browseWorkDir();
    void _apply();

private:
    void _syncDefaultCombo();
    void _saveCurrentFormToProfile();
    void _loadProfileToForm(const Profile& p);

    // Local working copies — not committed to KTermSettings until Apply/OK.
    QMap<QString, Profile> _localProfiles;
    QString _editingKey; // key of the profile currently shown in the form

    // General tab
    QComboBox*     _defaultCombo  = nullptr;

    // Profiles tab
    QListWidget*   _profileList   = nullptr;
    QPushButton*   _removeBtn     = nullptr;
    QGroupBox*     _formGroup     = nullptr;
    QLineEdit*     _shellEdit     = nullptr;
    QLineEdit*     _workDirEdit   = nullptr;
    QFontComboBox* _fontCombo     = nullptr;
    QSpinBox*      _fontSizeSpin  = nullptr;
    QComboBox*     _schemeCombo   = nullptr;
};

} // namespace KTerm
