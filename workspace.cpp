#include "workspace.h"
#include "expression.h"

enum class workspace::mode_type: int {
    normal = 0,
    insert = 1,
};

worksheet workspace::ws;
workspace::mode_type workspace::mode;
const std::optional<terminal::rgb_color> workspace::error_color = {{255, 0, 0}};
bool workspace::mark_flush = true;
std::string workspace::insert_str;
bool workspace::insert_parse_error = false;

void workspace::render() {
    ws.update_col_start();
    ws.update_row_start();

    if (mark_flush) {
        terminal::clear();
        ws.bufsize = terminal::getSize() - terminal::size{ 1, 0 };
        ws.redraw();
        mark_flush = false;
    }

    for (worksheet::row_reference r(0); r.number<worksheet::MAX_ROW; ++r) {
        for (worksheet::col_reference c(0); c.number<worksheet::MAX_COL; ++c) {
            worksheet::cell& cell = ws.cells[r][c];
            if (cell.needs_redraw) {
                ws.draw_cell_text(worksheet::cell_reference(r, c));
                cell.needs_redraw = false;
            }
        }
    }

    if (mode == mode_type::insert) {
        std::string message = "Edit " + ws.active_cell.to_code() + ": " + insert_str;
        terminal::set(terminal::getSize().row - 1, 0, message);
        for (int i=message.length(); i<terminal::getSize().col; ++i) {
            terminal::set(terminal::getSize().row-1, i, ' ');
        }
        if (insert_parse_error) {
            std::string error_message = "<- Parse Error";
            terminal::set(terminal::getSize().row-1, message.length() + 2, error_message, error_color);
            insert_parse_error = false;
        }
        terminal::cursor_pos = { terminal::getSize().row-1, message.length() };
    } else {
        terminal::cursor_pos = { 0, 0 };
        for (int i=0; i<terminal::getSize().col; ++i) {
            terminal::set(terminal::getSize().row-1, i, ' ');
        }
    }
}

bool workspace::isWordChar(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
}
void workspace::action(char ch) {
    if (ch == '\x0C') { // ^L
        mark_flush = true;
    } else if (mode == mode_type::normal) {
        if (ch == 'j' || ch == 'k' || ch == 'l' || ch == 'h') {
            std::pair<int, int> offset;
            if (ch == 'j') offset = {1, 0};
            else if (ch == 'k') offset = {-1, 0};
            else if (ch == 'l') offset = {0, 1};
            else if (ch == 'h') offset = {0, -1};

            worksheet::cell_reference newValue(ws.active_cell.row + offset.first, ws.active_cell.col + offset.second);
            if (newValue.col.number >= 0 && newValue.col.number < worksheet::MAX_COL && 
                    newValue.row.number >= 0 && newValue.row.number < worksheet::MAX_ROW) {
                ws.update_active_cell(ws.active_cell, newValue);
                ws.active_cell = newValue;
            }
        } else if (ch == 'i') {
            mode = mode_type::insert;
            insert_str = ws.cells[ws.active_cell].raw;
        }
    } else {
        if (ch == '\x7F' || ch == '\x08') { // DEL, BS (^H)
            if (insert_str.length() != 0) insert_str.pop_back();
        } else if (ch == '\x17') { // ^W
            while (insert_str.length() != 0 && !isWordChar(insert_str[insert_str.length() - 1])) {
                insert_str.pop_back();
            }
            while (insert_str.length() != 0 && isWordChar(insert_str[insert_str.length() - 1])) {
                insert_str.pop_back();
            }
        } else if (ch == '\x0A') { // LF (^J, Enter)
            if (insert_str.size() != 0 && insert_str[0] == '=') {
                try {
                    expression::parse(insert_str.substr(1, insert_str.length() - 1));
                } catch (expression::parse_exception e) {
                    insert_parse_error = true;
                    return;
                }
            }
            ws.cells[ws.active_cell].raw = insert_str;
            mode = mode_type::normal;
            insert_str = "";
            ws.recalculate();
        } else if (ch == '\x1B') { // ESC (^[)
            mode = mode_type::normal;
            insert_str = "";
        } else if (ch >= '\x20') { // All characters before (space) are control characters
            insert_str += ch;
        }
    }
}
