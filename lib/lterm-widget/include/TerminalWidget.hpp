#pragma once

#include "LTerminal.hpp"
#include "TextBuffer.hpp"
#include "ColorScheme.hpp"
#include "Profile.hpp"

#include <QAbstractScrollArea>
#include <QFont>
#include <QFontMetricsF>
#include <QOpenGLWidget>
#include <QTimer>

namespace LTerm {

class TerminalGLView;

/**
 * TerminalWidget — a QAbstractScrollArea that renders a LTerminal.
 *
 * The viewport is a TerminalGLView (QOpenGLWidget), so all cell painting
 * runs on the GPU via Qt's OpenGL paint engine.
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

    LTerminal* terminal() const { return _terminal; }

    /** Called by TerminalGLView::paintGL() — do not call directly. */
    void _paintGL(QOpenGLWidget* surface);

signals:
    void titleChanged(const QString& title);

protected:
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

    void _paintCell(QPainter& p, int row, int col, const TextCell& cell,
                    bool isCursor) const;

    LTerminal*      _terminal = nullptr;
    TerminalGLView* _glView   = nullptr;

    ColorScheme  _colorScheme;

    QFont        _font;
    QFontMetricsF _fm;
    int          _cellW  = 8;
    int          _cellH  = 16;
    int          _baselineOffset = 13;

    int          _cols = 80;
    int          _rows = 24;

    int          _scrollOffset = 0;

    bool         _cursorVisible = true;
    QTimer*      _cursorBlinkTimer = nullptr;

    bool         _repaintPending = false;
    QTimer*      _repaintCoalescer = nullptr;
};

} // namespace LTerm

