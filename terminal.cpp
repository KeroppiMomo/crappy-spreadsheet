#include "terminal.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdexcept>
#include <iostream>

terminal::size terminal::size::operator +(const terminal::size& other) const noexcept {
    return { row + other.row, col + other.col };
}
terminal::size terminal::size::operator -(const terminal::size& other) const noexcept {
    return { row - other.row, col - other.col };
}

void terminal::ansi::flush() { std::cout << std::flush; }
void terminal::ansi::cursor_up(const int n) { std::cout << CSI << n << "A"; }
void terminal::ansi::cursor_down(const int n) { std::cout << CSI << n << "B"; }
void terminal::ansi::cursor_forward(const int n) { std::cout << CSI << n << "C"; }
void terminal::ansi::cursor_backward(const int n) { std::cout << CSI << n << "D"; }
void terminal::ansi::cursor_next_line(const int n) { std::cout << CSI << n << "E"; }
void terminal::ansi::cursor_prev_line(const int n) { std::cout << CSI << n << "E"; }
void terminal::ansi::cursor_col(const int n) { std::cout << CSI << n+1 << "F"; }
void terminal::ansi::cursor_pos(const int row, const int col) { std::cout << CSI << row+1 << ';' << col+1 << "H"; }
void terminal::ansi::erase_display_end() { std::cout << CSI << 'J'; }
void terminal::ansi::erase_display_begin() { std::cout << CSI << "1;J"; }
void terminal::ansi::erase_display() { std::cout << CSI << "2;J"; }
void terminal::ansi::erase_line_end() { std::cout << CSI << 'K'; }
void terminal::ansi::erase_line_begin() { std::cout << CSI << "1;K"; }
void terminal::ansi::erase_line() { std::cout << CSI << "2;K"; }
void terminal::ansi::scroll_up(const int n) { std::cout << CSI << n << 'S'; }
void terminal::ansi::scroll_down(const int n) { std::cout << CSI << n << 'S'; }
std::pair<int, int> terminal::ansi::report_cursor_flush() {
    std::cout << CSI << "6n";
    flush();
    char ch;
    bool esc = false, bracket = false, semicolon = false;
    std::string row = "", col = "";
    while ((ch = terminal::getch()) != 'R') {
        if (ch == '\033') esc = true;
        else if (ch == '[') bracket = true;
        else if (ch == ';') semicolon = true;
        else if (esc && bracket) {
            if (semicolon) {
                col += ch;
            } else {
                row += ch;
            }
        } else {
            esc = false, bracket = false, semicolon = false;
        }
    }

    return { std::stoi(row)-1, std::stoi(col)-1 };
}

bool terminal::rgb_color::operator ==(const rgb_color& other) const noexcept {
    return r == other.r && g == other.g && b == other.b;
}
bool terminal::rgb_color::operator !=(const rgb_color& other) const noexcept {
    return !(*this == other);
}

bool terminal::screen_cell::operator ==(const screen_cell& other) const noexcept {
    return ch == other.ch && fg == other.fg && bg == other.bg;
}
bool terminal::screen_cell::operator !=(const screen_cell& other) const noexcept {
    return !(*this == other);
}

terminal::screen_cell terminal::screen[1000][1000];
bool terminal::_unflushed_pos[1000][1000];
std::pair<int, int> terminal::cursor_pos = {0, 0};

terminal::size terminal::getSize() noexcept {
    winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    return { w.ws_row, w.ws_col };
}

void terminal::set(int r, int c, const std::string& st, const std::optional<rgb_color> fg, const std::optional<rgb_color> bg) noexcept(false) {
    if (r < 0 || r >= getSize().row || c < 0 || c + st.length() - 1 >= getSize().col)
        throw std::out_of_range("Position outside screen");
    for (int i=0; i<st.length(); ++i) {
        screen_cell data = { st[i], fg, bg };
        if (screen[r][c+i] != data) {
            screen[r][c+i] = data;
            _unflushed_pos[r][c+i] = true;
        }
    }
}
void terminal::set(int r, int c, char ch, const std::optional<rgb_color>& fg, const std::optional<rgb_color>& bg) noexcept(false) {
    set(r, c, std::string(1, ch), fg, bg);
}

void terminal::clear() noexcept {
    for (int i=0; i<getSize().row; ++i) {
        for (int j=0; j<getSize().col; ++j) {
            screen[i][j] = { ' ', {}, {} };
        }
    }

    ansi::erase_display();
}

void terminal::flush() noexcept {
    std::pair<int, int> cur_pos = { -1, -1 };
    for (int r=0; r<getSize().row; ++r) {
        for (int c=0; c<getSize().col; ++c) {
            if (!_unflushed_pos[r][c] && !(r == cursor_pos.first && c == cursor_pos.second)) continue;
            _unflushed_pos[r][c] = false;
            if (cur_pos.first != r || cur_pos.second != c) {
                ansi::cursor_pos(r, c);
            }

            screen_cell& cell = screen[r][c];

            std::cout << ansi::CSI;
            if (cell.fg.has_value() && cell.bg.has_value()) {
                std::cout << "38;2;" << cell.fg.value().r << ';' << cell.fg.value().g << ';' << cell.fg.value().b << ';';
                std::cout << "48;2;" << cell.bg.value().r << ';' << cell.bg.value().g << ';' << cell.bg.value().b;
            } else if (cell.fg.has_value()) {
                std::cout << "38;2;" << cell.fg.value().r << ';' << cell.fg.value().g << ';' << cell.fg.value().b;
            } else if (cell.bg.has_value()) {
                std::cout << "48;2;" << cell.bg.value().r << ';' << cell.bg.value().g << ';' << cell.bg.value().b;
            } else {
                std::cout << "0";
            }
            std::cout << "m" << cell.ch << ansi::CSI << "0m";

            cur_pos.second++;
            if (cur_pos.second == getSize().col) cur_pos = { cur_pos.first+1, 0 };
        }
    }

    ansi::cursor_pos(cursor_pos.first, cursor_pos.second);
}


void terminal::initTermios() {
    tcgetattr(0, &old); /* grab old terminal i/o settings */
    current = old; /* make new settings same as old settings */
    current.c_lflag &= ~ICANON; /* disable buffered i/o */
    current.c_lflag &= ~ECHO; /* set no echo mode */
    tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
}

void terminal::resetTermios() {
    tcsetattr(0, TCSANOW, &old);
}

char terminal::getch() noexcept {
    char ch;
    initTermios();
    ch = getchar();
    resetTermios();
    return ch;
}
