// Copyright (c) Sannel LLC.
// Licensed under the MIT license.

#pragma once

#include "TextCell.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace LTerm
{
    struct CursorPos
    {
        int row = 0;
        int col = 0;
    };

    struct CursorState
    {
        CursorPos pos;
        TextAttribute attr;
        bool originMode = false;
    };

    // A terminal text buffer: active screen + scrollback.
    // Rows are 0-indexed from the top of the visible area.
    class TextBuffer
    {
    public:
        using DirtyCallback = std::function<void()>;

        TextBuffer(int rows, int cols);

        // Resize the buffer. Attempts to preserve content.
        void Resize(int newRows, int newCols);

        int Rows() const noexcept { return _rows; }
        int Cols() const noexcept { return _cols; }

        // Current cursor position (clamped to buffer dimensions).
        CursorPos CursorPosition() const noexcept { return _cursor.pos; }

        // Read a single cell. Returns blank cell if out of bounds.
        const TextCell& CellAt(int row, int col) const noexcept;

        // Current SGR attributes used when printing.
        const TextAttribute& CurrentAttr() const noexcept { return _cursor.attr; }
        void SetAttr(const TextAttribute& attr) noexcept { _cursor.attr = attr; }

        // ── Print ────────────────────────────────────────────────────────────
        void PrintChar(char32_t ch);
        void PrintString(const std::u32string_view sv);

        // ── Cursor motion ────────────────────────────────────────────────────
        void CursorUp(int n = 1) noexcept;
        void CursorDown(int n = 1) noexcept;
        void CursorForward(int n = 1) noexcept;
        void CursorBackward(int n = 1) noexcept;
        void CursorTo(int row, int col) noexcept;   // absolute (1-based externally, 0-based here)
        void CursorToCol(int col) noexcept;
        void CursorToRow(int row) noexcept;
        void CarriageReturn() noexcept;
        void LineFeed();   // may scroll
        void ReverseLineFeed();
        void Backspace() noexcept;
        void Tab() noexcept;
        void SaveCursor() noexcept;
        void RestoreCursor() noexcept;

        // ── Erase ────────────────────────────────────────────────────────────
        void EraseInDisplay(int mode);  // 0=below, 1=above, 2=all, 3=scrollback+all
        void EraseInLine(int mode);     // 0=right, 1=left, 2=whole
        void EraseCharacters(int n);

        // ── Insert / Delete ──────────────────────────────────────────────────
        void InsertLines(int n);
        void DeleteLines(int n);
        void InsertChars(int n);
        void DeleteChars(int n);

        // ── Scroll region ───────────────────────────────────────────────────
        void SetScrollRegion(int top, int bottom); // 1-based
        void ScrollUp(int n = 1);
        void ScrollDown(int n = 1);

        // ── Callbacks ───────────────────────────────────────────────────────
        void SetDirtyCallback(DirtyCallback cb) { _dirtyCallback = std::move(cb); }

        // Scrollback lines (index 0 = oldest).
        const std::vector<std::vector<TextCell>>& Scrollback() const noexcept { return _scrollback; }

        // Cap the number of scrollback lines kept in memory.
        // 0 = unlimited (default).
        void SetMaxScrollback(int limit) noexcept { _maxScrollback = limit; }

    private:
        int _rows;
        int _cols;
        int _scrollTop    = 0;
        int _scrollBottom = 0; // inclusive, defaults to _rows-1

        std::vector<std::vector<TextCell>> _screen;
        std::vector<std::vector<TextCell>> _scrollback;
        int _maxScrollback = 0;  // 0 = unlimited

        CursorState _cursor;
        CursorState _savedCursor;

        static const TextCell _blankCell;

        DirtyCallback _dirtyCallback;

        void _markDirty();
        void _clampCursor() noexcept;
        std::vector<TextCell> _blankRow() const;
        void _scrollUpRegion(int n);
        void _scrollDownRegion(int n);
    };
}
