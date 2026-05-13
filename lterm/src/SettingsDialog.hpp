// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "LTermSettings.hpp"

#include <QDialog>
#include <QMap>

class QComboBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QStackedWidget;

namespace LTerm {

/**
 * SettingsDialog — modal dialog for editing profiles and global settings.
 *
 * Uses a Windows-Terminal-style sidebar (QListWidget) on the left and
 * a QStackedWidget on the right. Maintains a local copy of profiles that
 * is only pushed to LTermSettings when the user clicks Save.
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void _onNavItemClicked(QListWidgetItem* item);
    void _addProfile();
    void _removeProfile(const QString& profileName);
    void _browseShell();
    void _browseWorkDir();
    void _save();

private:
    // Sidebar nav
    void _buildNav();
    void _buildStartupPage();
    void _buildProfilePage(const QString& profileName);
    void _buildColorSchemesPage();
    void _rebuildProfileNavItems();
    void _selectNavItem(QListWidgetItem* item);

    void _saveCurrentFormToProfile();
    void _loadProfileToForm(const Profile& p);
    void _syncDefaultCombo();

    // Nav helpers
    QListWidgetItem* _addNavHeader(const QString& text);
    QListWidgetItem* _addNavItem(const QString& text, const QIcon& icon,
                                  const QString& data, int indent = 0);

    // Local working copies — not committed to LTermSettings until Save.
    QMap<QString, Profile> _localProfiles;
    QString _editingProfileKey; // profile key whose form is currently visible

    // Layout
    QListWidget*   _nav          = nullptr;
    QStackedWidget* _stack       = nullptr;

    // Startup page
    QComboBox*     _defaultCombo = nullptr;

    // Profile page (one shared page, reloaded on switch)
    int            _profilePageIdx = -1;
    QLabel*        _profileTitle  = nullptr;
    QLineEdit*     _shellEdit     = nullptr;
    QLineEdit*     _workDirEdit   = nullptr;
    QFontComboBox* _fontCombo     = nullptr;
    QSpinBox*      _fontSizeSpin  = nullptr;
    QComboBox*     _schemeCombo   = nullptr;
    QPushButton*   _deleteProfileBtn = nullptr;

    // Save button (bottom right)
    QPushButton*   _saveBtn       = nullptr;
};

} // namespace LTerm
