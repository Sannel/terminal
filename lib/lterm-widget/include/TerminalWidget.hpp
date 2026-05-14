#pragma once

#include "LTerminal.hpp"
#include "TextBuffer.hpp"
#include "ColorScheme.hpp"
#include "Profile.hpp"

#include <QAbstractScrollArea>
#include <QFont>
#include <QFontMetricsF>
#include <QPixmap>
#include <QTimer>

namespace LTerm {

/**
 * TerminalWidget — a QAbstractScrollArea that renders a LTerminal.
 *
 * Responsibilities:
 *  - Draw the visible terminal buffer via QPainter (cell-by-cell).
 *  - Optional background image with opacity, painted before cell drawing.
 *  - Handle keyboard input → VT sequence encoding → LTerminal::sendInput.
 *  - Resize: recalculate rows/cols from pixel size, notify LTerminal.
 *  - Scrollback: translate scrollbar position → buffer view offset.
 *  - Cursor blink via a QTimer.
 *  - Mouse selection and right-click copy/paste context menu.
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

    /** Apply a full profile (font + color scheme + background + cursor + padding). */
    void applyProfile(const Profile& profile, const ColorScheme& scheme);

    /** Replace just the color scheme. */
    void setColorScheme(const ColorScheme& scheme);

    LTerminal* terminal() const { return _terminal; }

signals:
    void titleChanged(const QString& title);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    bool viewportEvent(QEvent* event) override;

private:
    void _onRepaintNeeded();
    void _onTitleChanged(const QString& title);
    void _scheduleRepaint();
    void _updateScrollbar();
    void _recalcDimensions();
    void _applyFont(const QFont& font);

    void _paintCell(QPainter& p, int row, int col, const TextCell& cell,
                    bool isCursor) const;
    void _paintCursor(QPainter& p, const QRect& cellRect, const QColor& cursorColor) const;

    // ── Selection ────────────────────────────────────────────────────────────
    struct SelPoint { int row = 0; int col = 0; };
    SelPoint _pixelToCell(const QPoint& pos) const;
    bool     _cellInSelection(int row, int col) const;
    QString  _selectionText() const;
    void     _copySelection();
    void     _showContextMenu(const QPoint& globalPos);

    void _onViewportMousePress(QMouseEvent* event);
    void _onViewportMouseMove(QMouseEvent* event);
    void _onViewportMouseRelease(QMouseEvent* event);
    void _onViewportContextMenu(QContextMenuEvent* event);

    SelPoint _selAnchor;
    SelPoint _selCaret;
    bool     _hasSelection = false;
    bool     _selecting    = false;

    // ── Terminal state ───────────────────────────────────────────────────────
    LTerminal* _terminal = nullptr;

    ColorScheme  _colorScheme;
    QPixmap      _bgPixmap;       // background image (null = none)
    double       _bgOpacity = 0.0;

    QFont        _font;
    QFontMetricsF _fm;
    int          _cellW  = 8;
    int          _cellH  = 16;
    int          _baselineOffset = 13;

    int          _cols = 80;
    int          _rows = 24;
    int          _padding = 0;       // px padding around the terminal grid

    int          _scrollOffset = 0;

    bool         _cursorVisible = true;
    CursorShape  _cursorShape   = CursorShape::Bar;
    int          _cursorHeight  = 25;  // % for Underscore shape
    std::optional<QColor> _cursorColorOverride;
    IntenseTextStyle _intenseTextStyle = IntenseTextStyle::Bold;
    QTimer*      _cursorBlinkTimer = nullptr;

    bool         _repaintPending = false;
    QTimer*      _repaintCoalescer = nullptr;
};

} // namespace LTerm

