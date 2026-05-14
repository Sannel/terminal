// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "LTermSettings.hpp"

#include <QColor>
#include <QDialog>
#include <QMap>
#include <optional>

class QCheckBox;
class QComboBox;
class QFontComboBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;
class QStackedWidget;
class QTableWidget;

namespace LTerm {

/**
 * SettingsDialog — modal dialog for editing profiles and global settings.
 *
 * Mirrors the Windows Terminal settings UI structure:
 *   Startup | Interaction | Appearance | PROFILES | Color Schemes
 *
 * Maintains local copies of all settings and only pushes to LTermSettings
 * when the user clicks Save.
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
    void _browseBgImage();
    void _save();

private:
    // ── Nav building ─────────────────────────────────────────────────────────
    void _buildNav();
    void _buildStartupPage();
    void _buildInteractionPage();
    void _buildGlobalAppearancePage();
    void _buildProfilePage();
    void _buildColorSchemesPage();
    void _rebuildProfileNavItems();

    // ── Data sync helpers ─────────────────────────────────────────────────────
    void _saveCurrentFormToProfile();
    void _loadProfileToForm(const Profile& p);
    void _saveGlobalForms();
    void _loadGlobalToStartup();
    void _loadGlobalToInteraction();
    void _loadGlobalToAppearance();
    void _syncDefaultCombo();

    // ── Color button helpers ──────────────────────────────────────────────────
    void _connectColorBtn(QPushButton* btn, std::optional<QColor>& storage);
    static void _updateColorBtn(QPushButton* btn, const std::optional<QColor>& c);
    static std::optional<QColor> _pickColor(QPushButton* btn,
                                            const std::optional<QColor>& current);

    // ── Nav item factories ────────────────────────────────────────────────────
    QListWidgetItem* _addNavHeader(const QString& text);
    QListWidgetItem* _addNavItem(const QString& text, const QIcon& icon,
                                  const QString& data, int indent = 0);

    // ── Local working copies ──────────────────────────────────────────────────
    QMap<QString, Profile> _localProfiles;
    GlobalSettings         _localGlobal;
    QString                _editingProfileKey;

    // ── Shared layout ─────────────────────────────────────────────────────────
    QListWidget*    _nav   = nullptr;
    QStackedWidget* _stack = nullptr;
    QPushButton*    _saveBtn = nullptr;

    // ── Startup page ──────────────────────────────────────────────────────────
    QComboBox* _defaultCombo        = nullptr;
    QComboBox* _launchModeCombo     = nullptr;
    QSpinBox*  _initialRowsSpin     = nullptr;
    QSpinBox*  _initialColsSpin     = nullptr;
    QCheckBox* _centerOnLaunchCheck = nullptr;
    QCheckBox* _alwaysOnTopCheck    = nullptr;
    QCheckBox* _alwaysShowTabsCheck = nullptr;
    QComboBox* _tabWidthModeCombo   = nullptr;

    // ── Interaction page ──────────────────────────────────────────────────────
    QCheckBox* _copyOnSelectCheck  = nullptr;
    QCheckBox* _trimPasteCheck     = nullptr;
    QLineEdit* _wordDelimitersEdit = nullptr;

    // ── Global Appearance page ────────────────────────────────────────────────
    QComboBox* _themeCombo = nullptr;

    // ── Profile page ─────────────────────────────────────────────────────────
    int    _profilePageIdx = -1;
    QLabel* _profileTitle  = nullptr;

    // General group
    QLineEdit*   _nameEdit           = nullptr;
    QLineEdit*   _shellEdit          = nullptr;
    QLineEdit*   _workDirEdit        = nullptr;
    QLineEdit*   _tabTitleEdit       = nullptr;
    QPushButton* _tabColorBtn        = nullptr;
    std::optional<QColor> _tabColor;
    QCheckBox*   _hiddenCheck        = nullptr;
    QCheckBox*   _suppressTitleCheck = nullptr;
    QComboBox*   _closeOnExitCombo   = nullptr;

    // Appearance group
    QComboBox*     _schemeCombo              = nullptr;
    QFontComboBox* _fontCombo                = nullptr;
    QSpinBox*      _fontSizeSpin             = nullptr;
    QComboBox*     _fontWeightCombo          = nullptr;
    QComboBox*     _cursorShapeCombo         = nullptr;
    QSpinBox*      _cursorHeightSpin         = nullptr;
    QPushButton*   _cursorColorBtn           = nullptr;
    std::optional<QColor> _cursorColor;
    QPushButton*   _fgColorBtn               = nullptr;
    std::optional<QColor> _fgColor;
    QPushButton*   _bgColorBtn               = nullptr;
    std::optional<QColor> _bgColor;
    QPushButton*   _selBgColorBtn            = nullptr;
    std::optional<QColor> _selBgColor;
    QCheckBox*     _enableBuiltinGlyphsCheck = nullptr;
    QCheckBox*     _enableColorGlyphsCheck   = nullptr;
    QLineEdit*     _bgImageEdit              = nullptr;
    QSpinBox*      _bgOpacitySpin            = nullptr;
    QComboBox*     _bgStretchCombo           = nullptr;
    QComboBox*     _bgAlignCombo             = nullptr;

    // Advanced group
    QSpinBox*    _historySizeSpin     = nullptr;
    QComboBox*   _scrollbarStateCombo = nullptr;
    QSpinBox*    _paddingSpin         = nullptr;
    QCheckBox*   _snapOnInputCheck    = nullptr;
    QCheckBox*   _altGrAliasingCheck  = nullptr;
    QTableWidget* _envVarsTable       = nullptr;
    QPushButton* _addEnvVarBtn        = nullptr;
    QPushButton* _removeEnvVarBtn     = nullptr;
    QPushButton* _deleteProfileBtn    = nullptr;
};

} // namespace LTerm
