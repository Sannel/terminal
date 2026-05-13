// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#include "TextBuffer.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace LTerm
{
    const TextCell TextBuffer::_blankCell{};

    TextBuffer::TextBuffer(int rows, int cols) :
        _rows(rows),
        _cols(cols),
        _scrollTop(0),
        _scrollBottom(rows - 1)
    {
        _screen.assign(static_cast<size_t>(rows), _blankRow());
    }

    void TextBuffer::Resize(int newRows, int newCols)
    {
        if (newRows < 1) { newRows = 1; }
        if (newCols < 1) { newCols = 1; }

        // Resize each existing row.
        for (auto& row : _screen)
        {
            row.resize(static_cast<size_t>(newCols), _blankCell);
        }

        // Update _cols now so _blankRow() produces rows of the correct width
        // when we add new rows below.
        _cols = newCols;

        // Add or remove rows at the bottom.
        while (static_cast<int>(_screen.size()) < newRows)
        {
            _screen.push_back(_blankRow());
        }
        _screen.resize(static_cast<size_t>(newRows), _blankRow());

        _rows = newRows;
        _scrollTop = 0;
        _scrollBottom = _rows - 1;
        _clampCursor();
        _markDirty();
    }

    const TextCell& TextBuffer::CellAt(int row, int col) const noexcept
    {
        if (row < 0 || row >= _rows || col < 0 || col >= _cols)
        {
            return _blankCell;
        }
        return _screen[static_cast<size_t>(row)][static_cast<size_t>(col)];
    }

    void TextBuffer::PrintChar(char32_t ch)
    {
        auto& row = _screen[static_cast<size_t>(_cursor.pos.row)];
        auto& cell = row[static_cast<size_t>(_cursor.pos.col)];
        cell.ch = ch;
        cell.attr = _cursor.attr;

        _cursor.pos.col++;
        if (_cursor.pos.col >= _cols)
        {
            _cursor.pos.col = 0;
            LineFeed();
        }
        _markDirty();
    }

    void TextBuffer::PrintString(const std::u32string_view sv)
    {
        for (const auto ch : sv)
        {
            PrintChar(ch);
        }
    }

    void TextBuffer::CursorUp(int n) noexcept
    {
        _cursor.pos.row = std::max(_scrollTop, _cursor.pos.row - n);
        _clampCursor();
    }

    void TextBuffer::CursorDown(int n) noexcept
    {
        _cursor.pos.row = std::min(_scrollBottom, _cursor.pos.row + n);
        _clampCursor();
    }

    void TextBuffer::CursorForward(int n) noexcept
    {
        _cursor.pos.col = std::min(_cols - 1, _cursor.pos.col + n);
    }

    void TextBuffer::CursorBackward(int n) noexcept
    {
        _cursor.pos.col = std::max(0, _cursor.pos.col - n);
    }

    void TextBuffer::CursorTo(int row, int col) noexcept
    {
        _cursor.pos.row = std::clamp(row, 0, _rows - 1);
        _cursor.pos.col = std::clamp(col, 0, _cols - 1);
    }

    void TextBuffer::CursorToCol(int col) noexcept
    {
        _cursor.pos.col = std::clamp(col, 0, _cols - 1);
    }

    void TextBuffer::CursorToRow(int row) noexcept
    {
        _cursor.pos.row = std::clamp(row, 0, _rows - 1);
    }

    void TextBuffer::CarriageReturn() noexcept
    {
        _cursor.pos.col = 0;
    }

    void TextBuffer::LineFeed()
    {
        if (_cursor.pos.row == _scrollBottom)
        {
            _scrollUpRegion(1);
        }
        else
        {
            _cursor.pos.row = std::min(_rows - 1, _cursor.pos.row + 1);
        }
        _markDirty();
    }

    void TextBuffer::ReverseLineFeed()
    {
        if (_cursor.pos.row == _scrollTop)
        {
            _scrollDownRegion(1);
        }
        else
        {
            _cursor.pos.row = std::max(0, _cursor.pos.row - 1);
        }
        _markDirty();
    }

    void TextBuffer::Backspace() noexcept
    {
        if (_cursor.pos.col > 0)
        {
            _cursor.pos.col--;
        }
    }

    void TextBuffer::Tab() noexcept
    {
        const int nextTab = ((_cursor.pos.col / 8) + 1) * 8;
        _cursor.pos.col = std::min(nextTab, _cols - 1);
    }

    void TextBuffer::SaveCursor() noexcept
    {
        _savedCursor = _cursor;
    }

    void TextBuffer::RestoreCursor() noexcept
    {
        _cursor = _savedCursor;
        _clampCursor();
    }

    void TextBuffer::EraseInDisplay(int mode)
    {
        switch (mode)
        {
        case 0: // erase below cursor
            EraseInLine(0); // cursor to end of current line
            for (int r = _cursor.pos.row + 1; r < _rows; ++r)
            {
                _screen[static_cast<size_t>(r)] = _blankRow();
            }
            break;
        case 1: // erase above cursor
            for (int r = 0; r < _cursor.pos.row; ++r)
            {
                _screen[static_cast<size_t>(r)] = _blankRow();
            }
            EraseInLine(1); // beginning of line to cursor
            break;
        case 2: // erase whole screen
        case 3: // erase whole screen + scrollback
            for (auto& row : _screen)
            {
                row = _blankRow();
            }
            if (mode == 3)
            {
                _scrollback.clear();
            }
            break;
        default:
            break;
        }
        _markDirty();
    }

    void TextBuffer::EraseInLine(int mode)
    {
        auto& row = _screen[static_cast<size_t>(_cursor.pos.row)];
        switch (mode)
        {
        case 0: // cursor to end
            for (int c = _cursor.pos.col; c < _cols; ++c)
            {
                row[static_cast<size_t>(c)] = _blankCell;
            }
            break;
        case 1: // beginning to cursor
            for (int c = 0; c <= _cursor.pos.col; ++c)
            {
                row[static_cast<size_t>(c)] = _blankCell;
            }
            break;
        case 2: // whole line
            row = _blankRow();
            break;
        default:
            break;
        }
        _markDirty();
    }

    void TextBuffer::EraseCharacters(int n)
    {
        auto& row = _screen[static_cast<size_t>(_cursor.pos.row)];
        const int end = std::min(_cursor.pos.col + n, _cols);
        for (int c = _cursor.pos.col; c < end; ++c)
        {
            row[static_cast<size_t>(c)] = _blankCell;
        }
        _markDirty();
    }

    void TextBuffer::InsertLines(int n)
    {
        // Insert n blank lines at the cursor row, within the scroll region.
        for (int i = 0; i < n; ++i)
        {
            _screen.insert(_screen.begin() + _cursor.pos.row, _blankRow());
            if (static_cast<int>(_screen.size()) > _rows)
            {
                _screen.erase(_screen.begin() + _scrollBottom + 1);
            }
        }
        _markDirty();
    }

    void TextBuffer::DeleteLines(int n)
    {
        // Delete n lines at cursor row, blank lines added at bottom of scroll region.
        for (int i = 0; i < n; ++i)
        {
            _screen.erase(_screen.begin() + _cursor.pos.row);
            _screen.insert(_screen.begin() + _scrollBottom, _blankRow());
        }
        _markDirty();
    }

    void TextBuffer::InsertChars(int n)
    {
        auto& row = _screen[static_cast<size_t>(_cursor.pos.row)];
        for (int i = 0; i < n; ++i)
        {
            row.insert(row.begin() + _cursor.pos.col, _blankCell);
            row.pop_back();
        }
        _markDirty();
    }

    void TextBuffer::DeleteChars(int n)
    {
        auto& row = _screen[static_cast<size_t>(_cursor.pos.row)];
        for (int i = 0; i < n && _cursor.pos.col < static_cast<int>(row.size()); ++i)
        {
            row.erase(row.begin() + _cursor.pos.col);
            row.push_back(_blankCell);
        }
        _markDirty();
    }

    void TextBuffer::SetScrollRegion(int top, int bottom)
    {
        // top/bottom are 1-based from the terminal; convert to 0-based.
        const int t = std::clamp(top - 1, 0, _rows - 2);
        const int b = std::clamp(bottom - 1, t + 1, _rows - 1);
        _scrollTop = t;
        _scrollBottom = b;
        // Cursor moves to top-left of screen.
        _cursor.pos = { 0, 0 };
    }

    void TextBuffer::ScrollUp(int n)
    {
        _scrollUpRegion(n);
        _markDirty();
    }

    void TextBuffer::ScrollDown(int n)
    {
        _scrollDownRegion(n);
        _markDirty();
    }

    // ── Private helpers ───────────────────────────────────────────────────────

    void TextBuffer::_markDirty()
    {
        if (_dirtyCallback)
        {
            _dirtyCallback();
        }
    }

    void TextBuffer::_clampCursor() noexcept
    {
        _cursor.pos.row = std::clamp(_cursor.pos.row, 0, _rows - 1);
        _cursor.pos.col = std::clamp(_cursor.pos.col, 0, _cols - 1);
    }

    std::vector<TextCell> TextBuffer::_blankRow() const
    {
        return std::vector<TextCell>(static_cast<size_t>(_cols), _blankCell);
    }

    void TextBuffer::_scrollUpRegion(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            // Save scrolled-off line to scrollback.
            _scrollback.push_back(_screen[static_cast<size_t>(_scrollTop)]);

            // Shift rows up within the scroll region.
            for (int r = _scrollTop; r < _scrollBottom; ++r)
            {
                _screen[static_cast<size_t>(r)] = std::move(_screen[static_cast<size_t>(r + 1)]);
            }
            _screen[static_cast<size_t>(_scrollBottom)] = _blankRow();
        }
    }

    void TextBuffer::_scrollDownRegion(int n)
    {
        for (int i = 0; i < n; ++i)
        {
            // Shift rows down within the scroll region.
            for (int r = _scrollBottom; r > _scrollTop; --r)
            {
                _screen[static_cast<size_t>(r)] = std::move(_screen[static_cast<size_t>(r - 1)]);
            }
            _screen[static_cast<size_t>(_scrollTop)] = _blankRow();
        }
    }
}
