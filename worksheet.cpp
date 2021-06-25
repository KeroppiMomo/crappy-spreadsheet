#include "worksheet.h"

std::shared_ptr<const expression::primitive> worksheet::cell::calculate() {
    if (calculation_state == calculation_state_type::finished) return value;
    if (calculation_state == calculation_state_type::in_progress) throw std::make_shared<expression::error>(expression::error::values::recur);
    calculation_state = calculation_state_type::in_progress;
    try {
        std::string::size_type size;
        std::shared_ptr<expression::primitive> res = std::make_shared<expression::integer>(std::stoll(raw, &size));
        if (size != raw.size()) throw std::invalid_argument("expect size == trimmed.size()");
        expr = res;
        needs_redraw = value != res;
        value = res;
        calculation_state = calculation_state_type::finished;
        return res;
    } catch (std::exception e) {}
    
    if (raw.size() == 0 || raw[0] != '=') {
        std::shared_ptr<expression::primitive> res = std::make_shared<expression::text>(raw);
        expr = res;
        needs_redraw = value != res;
        value = res;
        calculation_state = calculation_state_type::finished;
        return res;
    }

    expr = expression::parse(raw.substr(1, raw.length() - 1));
    std::shared_ptr<const expression::primitive> res;
    try {
        res = expr->evaluate();
    } catch (std::shared_ptr<expression::error> e) {
        res = e;
        needs_redraw = value != res;
        value = res;
        calculation_state = calculation_state_type::finished;
        throw e;
    }

    needs_redraw = value != res;
    value = res;
    calculation_state = calculation_state_type::finished;
    return res;
}

worksheet::worksheet() {
    for (int i=0; i<MAX_ROW; ++i) col_width[i] = 10;
    for (int i=0; i<MAX_COL; ++i) row_height[i] = 3;
}

void worksheet::update_row_start() {
    for (int i=0, r=header_row_height+1; i<MAX_ROW; r += row_height[i] + 1, i++) {
        row_start[i] = r;
    }
}
void worksheet::update_col_start() {
    for (int i=0, c=header_col_width+1; i<MAX_COL; c += col_width[i] + 1, i++) {
        col_start[i] = c;
    }
}

void worksheet::draw_row_lines() {
    for (int i=0; i<MAX_ROW; ++i) {
        int r = row_start[i]-1;
        if (r >= bufsize.row) break;
        for (int c=0; c<bufsize.col; ++c) {
            terminal::set(r, c, ' ', {}, border_color);
        }
    }
}

void worksheet::draw_col_lines() {
    for (int i=0; i<MAX_COL; ++i) {
        int c = col_start[i]-1;
        if (c >= bufsize.col) break;
        for (int r=0; r<bufsize.row; ++r) {
            terminal::set(r, c, ' ', {}, border_color);
        }
    }
}

void worksheet::draw_header_row(col_reference col, bool is_active) {
    std::optional<terminal::rgb_color> bg, fg;
    if (is_active) {
        bg = active_header_bg_color;
        fg = active_header_fg_color;
    } else {
        bg = header_bg_color;
        fg = header_fg_color;
    }

    for (int j=0; j<std::min(bufsize.row, header_row_height); ++j) {
        for (int k=col_start[col.number]; k<std::min(bufsize.col, col_start[col.number]+col_width[col.number]); ++k) {
            terminal::set(j, k, ' ', {}, bg);
        }
    }

    int width = std::min(col_width[col.number], bufsize.col - col_start[col.number]);
    std::string code = col.to_code();
    try {
        if ((int)code.length() > width) { // Yes, that (int) costs me 15 mintues of debugging
            for (int j=0; j<width; ++j) {
                terminal::set(header_row_height-1, col_start[col.number]+width-1-j, code[code.length()-1-j], fg, bg);
            }
        } else {
            for (int j=0; j<code.length(); ++j) {
                terminal::set(header_row_height-1, col_start[col.number]+(width-code.length())/2+j, code[j], fg, bg);
            }
        }
    } catch (std::out_of_range e) {
        std::cout << width << ' ' << code.length();
        exit(0);
    }
}

void worksheet::draw_header_col(row_reference row, bool is_active) {
    std::optional<terminal::rgb_color> bg, fg;
    if (is_active) {
        bg = active_header_bg_color;
        fg = active_header_fg_color;
    } else {
        bg = header_bg_color;
        fg = header_fg_color;
    }

    for (int j=0; j<std::min(bufsize.col, header_col_width); ++j) {
        for (int k=row_start[row.number]; k<std::min(bufsize.row, row_start[row.number]+row_height[row.number]); ++k) {
            terminal::set(k, j, ' ', {}, bg);
        }
    }

    int height = std::min(row_height[row.number], bufsize.row - row_start[row.number]);
    std::string code = row.to_code();
    if (row_start[row.number] + height/2 < bufsize.row) {
        for (int j=0; j<std::min((int)code.length(), header_col_width); ++j) {
            terminal::set(row_start[row.number] + height/2, header_col_width-1-j, code[code.length()-1-j], fg, bg);
        }
    }
}

void worksheet::draw_cell_borders(const cell_reference& cell, const std::optional<terminal::rgb_color> bg) {
    int active_r = row_start[cell.row.number];
    int active_c = col_start[cell.col.number];
    for (int r=active_r-1; r<=std::min(bufsize.row-1, active_r+row_height[cell.row.number]); ++r) {
        if (active_c-1 < bufsize.col)
            terminal::set(r, active_c-1, ' ', {}, bg);
        if (active_c + col_width[cell.col.number] < bufsize.col)
            terminal::set(r, active_c+col_width[cell.col.number], ' ', {}, bg);
    }
    for (int c=active_c-1; c<=std::min(bufsize.col-1, active_c+col_width[cell.col.number]); ++c) {
        if (active_r-1 < bufsize.row)
            terminal::set(active_r-1, c, ' ', {}, bg);
        if (active_r + row_height[cell.row.number] < bufsize.row)
            terminal::set(active_r+row_height[cell.row.number], c, ' ', {}, bg);
    }
}
void worksheet::draw_active_borders(const cell_reference& active_cell) {
    draw_cell_borders(active_cell, active_border_color);
}

void worksheet::draw_cell_text(const cell_reference& cell) {
    int width = std::min(col_width[cell.col.number], bufsize.col - col_start[cell.col.number]);
    int height = std::min(row_height[cell.row.number], bufsize.row - row_start[cell.row.number]);
    if (width <= 0 || height <= 0) return;
    std::string content = cells[cell].value->cell_value(width);
    terminal::set(row_start[cell.row.number] + height/2, col_start[cell.col.number], content);
}

void worksheet::redraw() {
    draw_row_lines();
    draw_col_lines();

    if (header_row_height > 0) {
        for (col_reference i(0); i.number<MAX_COL; ++i) {
            if (col_start[i.number] >= bufsize.col) break;
            draw_header_row(i, i == active_cell.col);
        }
    }

    if (header_col_width > 0) {
        for (row_reference i(0); i.number<MAX_ROW; ++i) {
            if (row_start[i.number] >= bufsize.row) break;
            draw_header_col(i, i == active_cell.row);
        }
    }

    draw_active_borders(active_cell);

    for (row_reference r(0); r.number<MAX_ROW; ++r) {
        if (row_start[r.number] >= bufsize.row) break;
        for (col_reference c(0); c.number<MAX_COL; ++c) {
            if (col_start[c.number] >= bufsize.col) break;
            draw_cell_text(cell_reference(r, c));
        }
    }
}


void worksheet::update_active_cell(const cell_reference& oldValue, const cell_reference& newValue) {
    if (oldValue == newValue) return;
    draw_cell_borders(oldValue, border_color);
    draw_header_col(oldValue.row, false);
    draw_header_row(oldValue.col, false);
    draw_cell_borders(newValue, active_border_color);
    draw_header_col(newValue.row, true);
    draw_header_row(newValue.col, true);
}

void worksheet::update_needs_redraw_cell() {
    for (row_reference r(0); r.number<MAX_ROW; ++r) {
        for (col_reference c(0); c.number<MAX_COL; ++c) {
            if (!cells[r][c].needs_redraw) continue;
            draw_cell_text(cell_reference(r, c));
        }
    }
}

void worksheet::recalculate() {
    for (row_reference r(0); r.number<MAX_ROW; ++r) {
        for (col_reference c(0); c.number<MAX_COL; ++c) {
            cells[r][c].calculation_state = cell::calculation_state_type::pending;
            cells[r][c].needs_redraw = false;
        }
    }
    for (row_reference r(0); r.number<MAX_ROW; ++r) {
        for (col_reference c(0); c.number<MAX_COL; ++c) {
            try {
                cells[r][c].calculate();
            } catch (std::shared_ptr<expression::error> e) {}
        }
    }
}
