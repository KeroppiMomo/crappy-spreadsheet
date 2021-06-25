#ifndef __INCLUDE_TERMINAL_
#define __INCLUDE_TERMINAL_

#include <string>
#include <optional>
#include <termios.h>

/// Utility functions for drawing on the terminal screen.
namespace terminal { // Forward declare to keep struct declaration at the top level
    struct size;
    struct rgb_color;
    struct screen_cell;
};

/**
 * Size of the terminal.
 *
 * To retrieve the current terminal size, call `terminal::getSize`.
 */
struct terminal::size {
    /// Number of rows.
    int row;
    /// Number of columns.
    int col;

    terminal::size operator +(const terminal::size& other) const noexcept;
    terminal::size operator -(const terminal::size& other) const noexcept;
};

/// An RGB color of a terminal screen cell.
struct terminal::rgb_color {
    int r, g, b;

    rgb_color(int r, int g, int b): r(r), g(g), b(b) {}

    bool operator==(const rgb_color& other) const noexcept;
    bool operator!=(const rgb_color& other) const noexcept;
};

/// Information about a terminal screen cell.
struct terminal::screen_cell {
    /// Character in the cell.
    char ch;
    /// Foreground color, i.e. the text color.
    std::optional<rgb_color> fg;
    /// Background color, i.e. the cell color.
    std::optional<rgb_color> bg;

    bool operator==(const screen_cell& other) const noexcept;
    bool operator!=(const screen_cell& other) const noexcept;
};

namespace terminal {
    struct size;
    struct rgb_color;
    struct screen_cell;

    /**
     * Screen cells buffer to be flushed to the screen.
     *
     * Position index is taken as (row, column), with the topmost row being row 0 and the leftmost column being column 0.
     */
    extern screen_cell screen[1000][1000];

    /**
     * True if the cell at that position is to be flushed to the screen. 
     * Otherwise, the flushed cell is up to date with the cell.
     *
     * Position index is taken as (row, column), with the topmost row being
     * row 0 and the leftmost column being column 0.
     */
    extern bool _unflushed_pos[1000][1000];

    /**
     * Cursor position to be flushed to the screen.
     *
     * Position index is taken as (row, column), with the topmost row being
     * row 0 and the leftmost column being column 0.
     */
    extern std::pair<int, int> cursor_pos;

    /**
     * Retrieve the current screen size.
     *
     * This is implemeneted with the system `ioctl` function.
     */
    size getSize() noexcept;

    /**
     * Write a string `st` on the screen buffer starting from a screen position.
     * The string is written with left align to the position.
     *
     * @param r Row index to write the string, with the topmost row being row 0.
     * @param c Column index to write the string, with the leftmost row being 
     *     column 0.
     * @param st String to be shown.
     * @param fg Optional foreground color, i.e. the text color.
     * @param bg Optional background color, i.e. the cell color.
     * @throws std::out_of_range Thrown if any character to be written is out of
     *     current screen bound.
     * @exceptsafe Strong exception safety. Out of bound check is performed before
     *     any modification.
     */
    void set(int r, int c, const std::string& st, const std::optional<rgb_color> fg = {}, const std::optional<rgb_color> bg = {}) noexcept(false);
    /**
     * Write a character `ch` on the screen buffer at a screen position.
     *
     * @param r Row index to write the character, with the topmost row being
     *     row 0.
     * @param c Column index to write the character, with the leftmost row being
     *     column 0.
     * @param ch Character to be shown.
     * @param fg Optional foreground color, i.e. the text color.
     * @param bg Optional background color, i.e. the cell color.
     * @throws std::out_of_range Thrown if the character to be written is out of
     *     current screen bound.
     * @exceptsafe Strong exception safety. Out of bound check is performed before
     *     any modification.
     */
    void set(int r, int c, char ch, const std::optional<rgb_color>& fg = {}, const std::optional<rgb_color>& bg = {}) noexcept(false);

    /**
     * Clear both the terminal screen and screen buffer.
     */
    void clear() noexcept;

    /**
     * Flush the screen buffer to the terminal screen.
     */
    void flush() noexcept;

    /**
     * Termios object for `terminal::getch`.
     * @see initTermios
     * @see resetTermios
     * @see getch
     */
    static struct termios old, current;

    // Copied from https://stackoverflow.com/questions/7469139/what-is-the-equivalent-to-getch-getche-in-linux
    /* Initialize new terminal i/o settings */
    void initTermios();

    /* Restore old terminal i/o settings */
    void resetTermios();

    /**
     * Wait and read a character from key input.
     *
     * This is synchronous and blocks the main thread.
     */
    char getch() noexcept;
}

/** Define CSI escape keys operations in ANSI.
 * @see [Wikipedia ANSI CSI sequences](https://en.wikipedia.org/wiki/ANSI_escape_code#CSI_(Control_Sequence_Introducer)_sequences)
 */
namespace terminal::ansi {
    /// Escape sequence to begin a CSI.
    const std::string CSI = "\033[";

    /// Flush the content in `std::cout`. This is not a CSI sequence but idk why i put it here.
    void flush();
    void cursor_up(const int n = 1);
    void cursor_down(const int n = 1);
    void cursor_forward(const int n = 1);
    void cursor_backward(const int n = 1);
    void cursor_next_line(const int n = 1);
    void cursor_prev_line(const int n = 1);
    void cursor_col(const int n = 0);
    void cursor_pos(const int row = 0, const int col = 0);
    void erase_display_end();
    void erase_display_begin();
    void erase_display();
    void erase_line_end();
    void erase_line_begin();
    void erase_line();
    void scroll_up(const int n = 1);
    void scroll_down(const int n = 1);
    std::pair<int, int> report_cursor_flush();
};

#endif
