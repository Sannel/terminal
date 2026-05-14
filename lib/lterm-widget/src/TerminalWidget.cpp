// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "TerminalWidget.hpp"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenu>
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
        viewport()->update();
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
    viewport()->setMouseTracking(true);
    verticalScrollBar()->setSingleStep(1);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int value) {
        const int maxVal = verticalScrollBar()->maximum();
        _scrollOffset = maxVal - value;
        viewport()->update();
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

    // Cursor settings.
    _cursorShape        = profile.cursorShape;
    _cursorHeight       = profile.cursorHeight;
    _cursorColorOverride= profile.cursorColor;

    // Padding.
    _padding = std::max(0, profile.padding);

    // Scrollbar policy.
    switch (profile.scrollbarState) {
    case ScrollbarState::Hidden: setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff); break;
    case ScrollbarState::Always: setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);  break;
    default:                     setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);  break;
    }

    // Background image.
    _bgOpacity = profile.backgroundImageOpacity;
    if (!profile.backgroundImagePath.isEmpty()) {
        _bgPixmap.load(profile.backgroundImagePath);
    } else {
        _bgPixmap = QPixmap{};
    }

    setColorScheme(scheme);
    _recalcDimensions();
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

void TerminalWidget::paintEvent(QPaintEvent* /*event*/)
{
    const auto& buf = _terminal->Buffer();
    QPainter p(viewport());
    p.setFont(_font);

    const int sbRows = static_cast<int>(buf.Scrollback().size());
    const int screenRows = buf.Rows();
    const QRect vr = viewport()->rect();

    // 1. Solid background.
    p.fillRect(vr, _colorScheme.background);

    // 2. Background image (if set and opacity > 0).
    if (!_bgPixmap.isNull() && _bgOpacity > 0.0) {
        p.save();
        p.setOpacity(_bgOpacity);
        // Scale to fill viewport while preserving aspect ratio.
        const QPixmap scaled = _bgPixmap.scaled(
            vr.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        const int x = (vr.width()  - scaled.width())  / 2;
        const int y = (vr.height() - scaled.height()) / 2;
        p.drawPixmap(x, y, scaled);
        p.restore();
    }

    const CursorPos cursorPos = buf.CursorPosition();

    for (int row = 0; row < _rows; ++row) {
        // _scrollOffset==0 = live view (bottom); positive = scrolled up N rows.
        // Negative bufRow means the row comes from scrollback.
        const int bufRow = row - _scrollOffset;

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
    const QRect rect(_padding + col * _cellW, _padding + row * _cellH,
                     cell.wide ? _cellW * 2 : _cellW, _cellH);

    const TextAttribute& attr = cell.attr;

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

    // For filled cursor shapes, invert fg/bg.
    const bool filledCursor = isCursor && (
        _cursorShape == CursorShape::FilledBox ||
        _cursorShape == CursorShape::Vintage);

    if (attr.inverse || filledCursor) {
        std::swap(fg, bg);
    }
    if (attr.invisible) {
        fg = bg;
    }
    if (attr.faint) {
        fg.setAlpha(128);
    }

    // Selection highlight overrides background (cursor cells are exempt).
    if (!isCursor && _cellInSelection(row, col)) {
        bg = _colorScheme.selectionBg;
    }

    // Background fill.
    if (bg != _colorScheme.background || filledCursor) {
        p.fillRect(rect, bg);
    }

    if (cell.ch != U' ' && cell.ch != 0) {
        QFont f = _font;
        if (attr.bold)   { f.setBold(true); }
        if (attr.italic) { f.setItalic(true); }
        p.setFont(f);
        p.setPen(fg);

        const QString text = QString::fromUcs4(reinterpret_cast<const char32_t*>(&cell.ch), 1);
        p.drawText(rect.left(), rect.top() + _baselineOffset, text);
    }

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

    // Draw non-filled cursor shapes on top of the rendered cell.
    if (isCursor && !filledCursor) {
        _paintCursor(p, rect, _cursorColorOverride.value_or(_colorScheme.foreground));
    }
}

void TerminalWidget::_paintCursor(QPainter& p, const QRect& r, const QColor& color) const
{
    p.save();
    p.setPen(color);
    switch (_cursorShape) {
    case CursorShape::Bar:
        p.fillRect(QRect(r.left(), r.top(), 2, r.height()), color);
        break;
    case CursorShape::Underscore: {
        const int barH = std::max(1, r.height() * _cursorHeight / 100);
        p.fillRect(QRect(r.left(), r.bottom() - barH + 1, r.width(), barH), color);
        break;
    }
    case CursorShape::EmptyBox:
        p.drawRect(r.adjusted(0, 0, -1, -1));
        break;
    default:
        break;
    }
    p.restore();
}

// ── Input ────────────────────────────────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent* event)
{
    const Qt::KeyboardModifiers mod = event->modifiers();
    const bool ctrl  = mod.testFlag(Qt::ControlModifier);
    const bool shift = mod.testFlag(Qt::ShiftModifier);
    const bool alt   = mod.testFlag(Qt::AltModifier);

    // Clipboard ops handled BEFORE scroll-reset so the user can copy/paste
    // without losing their scroll position or accidentally returning to live view.
    if (ctrl && shift) {
        if (event->key() == Qt::Key_C) {
            _copySelection();
            return;
        }
        if (event->key() == Qt::Key_V) {
            const QString text = QGuiApplication::clipboard()->text();
            if (!text.isEmpty()) {
                _terminal->SendInput(text.toUtf8().toStdString());
            }
            return;
        }
    }

    // Scroll back to live view on any other key.
    if (_scrollOffset != 0) {
        _scrollOffset = 0;
        verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    }

    QString seq;
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
    const int vw = std::max(0, viewport()->width()  - 2 * _padding);
    const int vh = std::max(0, viewport()->height() - 2 * _padding);

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

// ── Selection ────────────────────────────────────────────────────────────────

TerminalWidget::SelPoint TerminalWidget::_pixelToCell(const QPoint& pos) const
{
    const int col = std::clamp((pos.x() - _padding) / _cellW, 0, _cols - 1);
    const int row = std::clamp((pos.y() - _padding) / _cellH, 0, _rows - 1);
    return { row, col };
}

bool TerminalWidget::_cellInSelection(int row, int col) const
{
    if (!_hasSelection) return false;

    SelPoint start = _selAnchor;
    SelPoint end   = _selCaret;
    if (start.row > end.row || (start.row == end.row && start.col > end.col)) {
        std::swap(start, end);
    }

    if (row < start.row || row > end.row) return false;
    if (row == start.row && col < start.col) return false;
    if (row == end.row   && col > end.col)   return false;
    return true;
}

QString TerminalWidget::_selectionText() const
{
    if (!_hasSelection) return {};

    SelPoint start = _selAnchor;
    SelPoint end   = _selCaret;
    if (start.row > end.row || (start.row == end.row && start.col > end.col)) {
        std::swap(start, end);
    }

    const auto& buf  = _terminal->Buffer();
    const int sbRows = static_cast<int>(buf.Scrollback().size());

    QString result;
    for (int row = start.row; row <= end.row; ++row) {
        const int startCol = (row == start.row) ? start.col : 0;
        const int endCol   = (row == end.row)   ? end.col   : (_cols - 1);
        const int bufRow   = row - _scrollOffset;

        QString line;
        for (int col = startCol; col <= endCol; ++col) {
            const TextCell* cell = nullptr;
            TextCell dummy{};
            if (bufRow < 0) {
                const int sbIdx = sbRows + bufRow;
                if (sbIdx >= 0 && sbIdx < static_cast<int>(buf.Scrollback().size())) {
                    const auto& sbLine = buf.Scrollback()[sbIdx];
                    cell = (col < static_cast<int>(sbLine.size())) ? &sbLine[col] : &dummy;
                } else {
                    cell = &dummy;
                }
            } else if (bufRow < buf.Rows()) {
                cell = &buf.CellAt(bufRow, col);
            } else {
                cell = &dummy;
            }

            if (cell->ch != 0 && cell->ch != U' ') {
                line += QString::fromUcs4(reinterpret_cast<const char32_t*>(&cell->ch), 1);
            } else {
                line += QLatin1Char(' ');
            }
        }

        // Trim trailing spaces from each line.
        while (!line.isEmpty() && line.back() == QLatin1Char(' ')) {
            line.chop(1);
        }

        if (row < end.row) {
            result += line + QLatin1Char('\n');
        } else {
            result += line;
        }
    }

    return result;
}

void TerminalWidget::_copySelection()
{
    if (!_hasSelection) return;
    const QString text = _selectionText();
    if (text.isEmpty()) return;

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(text, QClipboard::Clipboard);
    if (clipboard->supportsSelection()) {
        clipboard->setText(text, QClipboard::Selection);
    }
}

void TerminalWidget::_showContextMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* copyAction  = menu.addAction(QStringLiteral("Copy"));
    QAction* pasteAction = menu.addAction(QStringLiteral("Paste"));
    copyAction->setEnabled(_hasSelection);

    const QAction* chosen = menu.exec(globalPos);
    if (chosen == copyAction) {
        _copySelection();
    } else if (chosen == pasteAction) {
        const QString text = QGuiApplication::clipboard()->text();
        if (!text.isEmpty()) {
            _terminal->SendInput(text.toUtf8().toStdString());
        }
    }
}

void TerminalWidget::_onViewportMousePress(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        _selAnchor    = _pixelToCell(event->pos());
        _selCaret     = _selAnchor;
        _hasSelection = false;
        _selecting    = true;
        viewport()->update();
    }
}

void TerminalWidget::_onViewportMouseMove(QMouseEvent* event)
{
    if (_selecting && (event->buttons() & Qt::LeftButton)) {
        _selCaret     = _pixelToCell(event->pos());
        _hasSelection = (_selCaret.row != _selAnchor.row || _selCaret.col != _selAnchor.col);
        viewport()->update();
    }
}

void TerminalWidget::_onViewportMouseRelease(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && _selecting) {
        _selCaret     = _pixelToCell(event->pos());
        _hasSelection = (_selCaret.row != _selAnchor.row || _selCaret.col != _selAnchor.col);
        _selecting    = false;

        // Auto-copy to X11 primary selection on release.
        if (_hasSelection) {
            QClipboard* clipboard = QGuiApplication::clipboard();
            if (clipboard->supportsSelection()) {
                clipboard->setText(_selectionText(), QClipboard::Selection);
            }
        }
        viewport()->update();
    }
}

void TerminalWidget::_onViewportContextMenu(QContextMenuEvent* event)
{
    _showContextMenu(event->globalPos());
}

bool TerminalWidget::viewportEvent(QEvent* event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        _onViewportMousePress(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseMove:
        _onViewportMouseMove(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::MouseButtonRelease:
        _onViewportMouseRelease(static_cast<QMouseEvent*>(event));
        return true;
    case QEvent::ContextMenu:
        _onViewportContextMenu(static_cast<QContextMenuEvent*>(event));
        return true;
    default:
        return QAbstractScrollArea::viewportEvent(event);
    }
}

} // namespace LTerm
