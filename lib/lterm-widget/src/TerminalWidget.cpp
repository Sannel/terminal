// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "TerminalWidget.hpp"
#include "TerminalGLView.hpp"

#include <QFontDatabase>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

namespace LTerm {

// ── 256-color cube (indices 16-255) ──────────────────────────────────────────

static QColor colorFrom256(const ColorScheme& scheme, uint8_t idx)
{
    if (idx < 16) {
        return scheme.ansiColors[idx];
    }
    if (idx >= 232) {
        const int v = 8 + (idx - 232) * 10;
        return QColor(v, v, v);
    }
    idx -= 16;
    const int b = idx % 6;
    const int g = (idx / 6) % 6;
    const int r = idx / 36;
    auto f = [](int c) { return c == 0 ? 0 : 55 + c * 40; };
    return QColor(f(r), f(g), f(b));
}

// ── TerminalWidget ────────────────────────────────────────────────────────────

TerminalWidget::TerminalWidget(QWidget* parent) :
    QAbstractScrollArea(parent),
    _colorScheme(ColorScheme::Default()),
    _font(QFontDatabase::systemFont(QFontDatabase::FixedFont)),
    _fm(_font)
{
    _font.setPointSize(11);
    _applyFont(_font);

    setFocusPolicy(Qt::StrongFocus);
    viewport()->setBackgroundRole(QPalette::NoRole);
    viewport()->setAutoFillBackground(false);

    // Install the OpenGL viewport for GPU-accelerated rendering.
    _glView = new TerminalGLView(this);
    setViewport(_glView);

    // Cursor blink: 500ms interval.
    _cursorBlinkTimer = new QTimer(this);
    _cursorBlinkTimer->setInterval(500);
    connect(_cursorBlinkTimer, &QTimer::timeout, this, [this]() {
        _cursorVisible = !_cursorVisible;
        _scheduleRepaint();
    });

    // Repaint coalescer: collapse multiple dirty signals into one update().
    _repaintCoalescer = new QTimer(this);
    _repaintCoalescer->setSingleShot(true);
    _repaintCoalescer->setInterval(16); // ~60fps cap
    connect(_repaintCoalescer, &QTimer::timeout, this, [this]() {
        _repaintPending = false;
        if (_glView) {
            _glView->update();
        } else {
            viewport()->update();
        }
        _updateScrollbar();
    });

    // Create the terminal (default size, resized in showEvent/resizeEvent).
    _terminal = new LTerminal(_rows, _cols, this);

    connect(_terminal, &LTerminal::repaintNeeded, this, &TerminalWidget::_onRepaintNeeded);
    connect(_terminal, &LTerminal::titleChanged, this, &TerminalWidget::_onTitleChanged);
    connect(_terminal, &LTerminal::terminated, this, [this]() {
        _scheduleRepaint();
    });

    // Vertical scrollbar only.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    verticalScrollBar()->setSingleStep(1);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        const int maxVal = verticalScrollBar()->maximum();
        _scrollOffset = maxVal - value;
        if (_glView) { _glView->update(); } else { viewport()->update(); }
    });
}

TerminalWidget::~TerminalWidget() = default;

void TerminalWidget::_applyFont(const QFont& font)
{
    _font = font;
    _fm   = QFontMetricsF(_font);
    _cellW           = static_cast<int>(std::ceil(_fm.horizontalAdvance(QLatin1Char('M'))));
    _cellH           = static_cast<int>(std::ceil(_fm.height()));
    _baselineOffset  = static_cast<int>(std::ceil(_fm.ascent()));
    setFont(_font);
}

void TerminalWidget::applyProfile(const Profile& profile, const ColorScheme& scheme)
{
    QFont f(profile.fontFamily, profile.fontSize);
    f.setStyleHint(QFont::Monospace);
    _applyFont(f);
    setColorScheme(scheme);
}

void TerminalWidget::setColorScheme(const ColorScheme& scheme)
{
    _colorScheme = scheme;
    _scheduleRepaint();
}

void TerminalWidget::Start(const QString& program, const QStringList& /*args*/,
                           const QString& workingDir)
{
    _recalcDimensions();
    const std::string cmd = program.isEmpty() ? std::string{} : program.toStdString();
    const std::string cwd = workingDir.isEmpty() ? std::string{} : workingDir.toStdString();
    _terminal->Start(cmd, cwd);
    _cursorBlinkTimer->start();
}

// ── Painting ─────────────────────────────────────────────────────────────────

void TerminalWidget::_paintGL(QOpenGLWidget* surface)
{
    const auto& buf = _terminal->Buffer();
    QPainter p(surface);
    p.setFont(_font);

    const int sbRows = static_cast<int>(buf.Scrollback().size());
    const int screenRows = buf.Rows();

    // Fill background using the color scheme.
    p.fillRect(surface->rect(), _colorScheme.background);

    const CursorPos cursorPos = buf.CursorPosition();

    for (int row = 0; row < _rows; ++row) {
        const int bufRow = row + _scrollOffset - sbRows;

        for (int col = 0; col < _cols; ++col) {
            const TextCell* cell = nullptr;
            TextCell dummy{};
            if (bufRow < 0) {
                const int sbIdx = sbRows + bufRow;
                const auto& sbLine = buf.Scrollback()[sbIdx];
                cell = (col < static_cast<int>(sbLine.size())) ? &sbLine[col] : &dummy;
            } else if (bufRow < screenRows) {
                cell = &buf.CellAt(bufRow, col);
            } else {
                cell = &dummy;
            }

            const bool isCursor = (_scrollOffset == 0) &&
                                  (_cursorVisible) &&
                                  (bufRow == cursorPos.row) &&
                                  (col == cursorPos.col);

            _paintCell(p, row, col, *cell, isCursor);
        }
    }
}

void TerminalWidget::_paintCell(QPainter& p, int row, int col,
                                const TextCell& cell, bool isCursor) const
{
    const QRect rect(col * _cellW, row * _cellH,
                     cell.wide ? _cellW * 2 : _cellW, _cellH);

    const TextAttribute& attr = cell.attr;

    // Resolve a TextColor to a QColor using the current scheme.
    auto resolveColor = [this](const TextColor& tc, bool isFg) -> QColor {
        switch (tc.type) {
        case ColorType::Default:
            return isFg ? _colorScheme.foreground : _colorScheme.background;
        case ColorType::Index16:
            return (tc.index() < 16) ? _colorScheme.ansiColors[tc.index()]
                                     : _colorScheme.foreground;
        case ColorType::Index256:
            return colorFrom256(_colorScheme, tc.index());
        case ColorType::RGB:
            return QColor(tc.r, tc.g, tc.b);
        }
        return isFg ? _colorScheme.foreground : _colorScheme.background;
    };

    QColor fg = resolveColor(attr.fg, true);
    QColor bg = resolveColor(attr.bg, false);

    if (attr.inverse || isCursor) {
        std::swap(fg, bg);
    }
    if (attr.invisible) {
        fg = bg;
    }
    if (attr.faint) {
        fg.setAlpha(128);
    }

    // Background fill (skip if default to avoid over-drawing).
    if (bg != _colorScheme.background || isCursor) {
        p.fillRect(rect, bg);
    }

    if (cell.ch == U' ' || cell.ch == 0) {
        if (attr.underline) {
            p.setPen(fg);
            p.drawLine(rect.left(), rect.bottom(), rect.right(), rect.bottom());
        }
        return;
    }

    QFont f = _font;
    if (attr.bold)   { f.setBold(true); }
    if (attr.italic) { f.setItalic(true); }
    p.setFont(f);
    p.setPen(fg);

    const QString text = QString::fromUcs4(reinterpret_cast<const char32_t*>(&cell.ch), 1);
    p.drawText(rect.left(), rect.top() + _baselineOffset, text);

    if (attr.underline) {
        p.setPen(fg);
        p.drawLine(rect.left(), rect.top() + _baselineOffset + 1,
                   rect.right(), rect.top() + _baselineOffset + 1);
    }
    if (attr.strikethrough) {
        const int midY = rect.top() + _baselineOffset / 2;
        p.setPen(fg);
        p.drawLine(rect.left(), midY, rect.right(), midY);
    }
}

// ── Input ────────────────────────────────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent* event)
{
    // Scroll back to live view on any key.
    if (_scrollOffset != 0) {
        _scrollOffset = 0;
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    const Qt::KeyboardModifiers mod = event->modifiers();
    const bool ctrl  = mod.testFlag(Qt::ControlModifier);
    const bool shift = mod.testFlag(Qt::ShiftModifier);
    const bool alt   = mod.testFlag(Qt::AltModifier);

    QString seq;

    // Ctrl+Shift+C / Ctrl+Shift+V — clipboard (not sent to terminal).
    if (ctrl && shift && event->key() == Qt::Key_C) {
        // TODO: copy selection
        return;
    }
    if (ctrl && shift && event->key() == Qt::Key_V) {
        // TODO: paste from clipboard
        return;
    }

    // Ctrl+key → \x01–\x1A range.
    if (ctrl && !shift && !alt) {
        const int key = event->key();
        if (key >= Qt::Key_A && key <= Qt::Key_Z) {
            const char c = static_cast<char>(key - Qt::Key_A + 1);
            seq = QString(QChar(c));
        } else if (key == Qt::Key_BracketLeft)  { seq = QStringLiteral("\x1B"); }
        else if (key == Qt::Key_Backslash)       { seq = QStringLiteral("\x1C"); }
        else if (key == Qt::Key_BracketRight)    { seq = QStringLiteral("\x1D"); }
        else if (key == Qt::Key_AsciiCircum)     { seq = QStringLiteral("\x1E"); }
        else if (key == Qt::Key_Underscore)      { seq = QStringLiteral("\x1F"); }
        else if (key == Qt::Key_At)              { seq = QString(1, QChar(0)); }
    }

    // Special keys.
    if (seq.isEmpty()) {
        switch (event->key()) {
        case Qt::Key_Return:
        case Qt::Key_Enter:     seq = QStringLiteral("\r"); break;
        case Qt::Key_Backspace: seq = QStringLiteral("\x7F"); break;
        case Qt::Key_Tab:
            seq = shift ? QStringLiteral("\x1B[Z") : QStringLiteral("\t"); break;
        case Qt::Key_Escape:    seq = QStringLiteral("\x1B"); break;
        case Qt::Key_Up:        seq = QStringLiteral("\x1B[A"); break;
        case Qt::Key_Down:      seq = QStringLiteral("\x1B[B"); break;
        case Qt::Key_Right:     seq = QStringLiteral("\x1B[C"); break;
        case Qt::Key_Left:      seq = QStringLiteral("\x1B[D"); break;
        case Qt::Key_Home:      seq = QStringLiteral("\x1B[H"); break;
        case Qt::Key_End:       seq = QStringLiteral("\x1B[F"); break;
        case Qt::Key_PageUp:    seq = QStringLiteral("\x1B[5~"); break;
        case Qt::Key_PageDown:  seq = QStringLiteral("\x1B[6~"); break;
        case Qt::Key_Insert:    seq = QStringLiteral("\x1B[2~"); break;
        case Qt::Key_Delete:    seq = QStringLiteral("\x1B[3~"); break;
        case Qt::Key_F1:        seq = QStringLiteral("\x1BOP"); break;
        case Qt::Key_F2:        seq = QStringLiteral("\x1BOQ"); break;
        case Qt::Key_F3:        seq = QStringLiteral("\x1BOR"); break;
        case Qt::Key_F4:        seq = QStringLiteral("\x1BOS"); break;
        case Qt::Key_F5:        seq = QStringLiteral("\x1B[15~"); break;
        case Qt::Key_F6:        seq = QStringLiteral("\x1B[17~"); break;
        case Qt::Key_F7:        seq = QStringLiteral("\x1B[18~"); break;
        case Qt::Key_F8:        seq = QStringLiteral("\x1B[19~"); break;
        case Qt::Key_F9:        seq = QStringLiteral("\x1B[20~"); break;
        case Qt::Key_F10:       seq = QStringLiteral("\x1B[21~"); break;
        case Qt::Key_F11:       seq = QStringLiteral("\x1B[23~"); break;
        case Qt::Key_F12:       seq = QStringLiteral("\x1B[24~"); break;
        default: break;
        }
    }

    if (seq.isEmpty()) {
        const QString text = event->text();
        if (!text.isEmpty()) {
            // Prefix printable text with ESC if Alt is held.
            seq = alt ? (QStringLiteral("\x1B") + text) : text;
        }
    }

    if (!seq.isEmpty()) {
        _terminal->SendInput(seq.toStdString());
    }
}

// ── Resize ────────────────────────────────────────────────────────────────────

void TerminalWidget::resizeEvent(QResizeEvent* event)
{
    QAbstractScrollArea::resizeEvent(event);
    _recalcDimensions();
}

void TerminalWidget::_recalcDimensions()
{
    const int vw = viewport()->width();
    const int vh = viewport()->height();

    const int newCols = std::max(1, vw / _cellW);
    const int newRows = std::max(1, vh / _cellH);

    if (newCols != _cols || newRows != _rows) {
        _cols = newCols;
        _rows = newRows;
        _terminal->Resize(_rows, _cols);
    }

    _updateScrollbar();
}

// ── Scrollbar ────────────────────────────────────────────────────────────────

void TerminalWidget::_updateScrollbar()
{
    const int sbRows = static_cast<int>(_terminal->Buffer().Scrollback().size());
    verticalScrollBar()->setRange(0, sbRows);
    verticalScrollBar()->setValue(sbRows - _scrollOffset);
    verticalScrollBar()->setPageStep(_rows);
}

// ── Wheel ────────────────────────────────────────────────────────────────────

void TerminalWidget::wheelEvent(QWheelEvent* event)
{
    const int delta = event->angleDelta().y();
    const int steps = delta / 40; // ~3 rows per notch
    if (steps != 0) {
        const int newVal = verticalScrollBar()->value() - steps;
        verticalScrollBar()->setValue(
            std::clamp(newVal, verticalScrollBar()->minimum(),
                       verticalScrollBar()->maximum()));
    }
}

// ── Focus ─────────────────────────────────────────────────────────────────────

void TerminalWidget::focusInEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusInEvent(event);
    _cursorVisible = true;
    _cursorBlinkTimer->start();
    _scheduleRepaint();
}

void TerminalWidget::focusOutEvent(QFocusEvent* event)
{
    QAbstractScrollArea::focusOutEvent(event);
    _cursorBlinkTimer->stop();
    _cursorVisible = true; // always show cursor when unfocused
    _scheduleRepaint();
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void TerminalWidget::_onRepaintNeeded()
{
    _scheduleRepaint();
}

void TerminalWidget::_onTitleChanged(const QString& title)
{
    // Propagate to parent window if possible.
    if (auto* w = window()) {
        w->setWindowTitle(title);
    }
}

void TerminalWidget::_scheduleRepaint()
{
    if (!_repaintPending) {
        _repaintPending = true;
        _repaintCoalescer->start();
    }
}

} // namespace LTerm
