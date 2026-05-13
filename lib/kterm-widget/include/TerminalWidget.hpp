#pragma once

#include "KTerminal.hpp"
#include "TextBuffer.hpp"
#include "ColorScheme.hpp"
#include "Profile.hpp"

#include <QAbstractScrollArea>
#include <QFont>
#include <QFontMetricsF>
#include <QTimer>

namespace KTerm {

/**
 * TerminalWidget — a QAbstractScrollArea that renders a KTerminal.
 *
 * Responsibilities:
 *  - Draw the visible terminal buffer via QPainter (cell-by-cell).
 *  - Handle keyboard input → VT sequence encoding → KTerminal::sendInput.
 *  - Resize: recalculate rows/cols from pixel size, notify KTerminal.
 *  - Scrollback: translate scrollbar position → buffer view offset.
 *  - Cursor blink via a QTimer.
 */
class TerminalWidget : public QAbstractScrollArea
{
    Q_OBJECT

public:
    explicit TerminalWidget(QWidget* parent = nullptr);
    ~TerminalWidget() override;

    /** Start the terminal running the given program (default: $SHELL). */
    void Start(const QString& program = {}, const QStringList& args = {},
               const QString& workingDir = {});

    /** Apply a full profile (font + color scheme). */
    void applyProfile(const Profile& profile, const ColorScheme& scheme);

    /** Replace just the color scheme. */
    void setColorScheme(const ColorScheme& scheme);

    KTerminal* terminal() const { return _terminal; }

signals:
    void titleChanged(const QString& title);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void _onRepaintNeeded();
    void _onTitleChanged(const QString& title);
    void _scheduleRepaint();
    void _updateScrollbar();
    void _recalcDimensions();
    void _applyFont(const QFont& font);

    /** Paint a single cell at (col, row) in viewport coords using the given QPainter. */
    void _paintCell(QPainter& p, int row, int col, const TextCell& cell,
                    bool isCursor) const;

    KTerminal* _terminal = nullptr;

    ColorScheme  _colorScheme;

    QFont        _font;
    QFontMetricsF _fm;
    int          _cellW  = 8;
    int          _cellH  = 16;
    int          _baselineOffset = 13; // px from top of cell to text baseline

    int          _cols = 80;
    int          _rows = 24;

    int          _scrollOffset = 0;  // rows scrolled back (0 = live view)

    bool         _cursorVisible = true;
    QTimer*      _cursorBlinkTimer = nullptr;

    bool         _repaintPending = false;
    QTimer*      _repaintCoalescer = nullptr;
};

} // namespace KTerm
