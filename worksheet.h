#ifndef __INCLUDE_WORKSHEET_
#define __INCLUDE_WORKSHEET_

#include "terminal.h"
#include "worksheet_reference.h"
#include "expression.h"
#include <iostream>
#include <string>
#include <array>

class worksheet: public worksheet_reference {
    public:
        static const int MAX_ROW = 100;
        static const int MAX_COL = 100;
        using worksheet_reference::reference;
        using worksheet_reference::half_reference;
        using worksheet_reference::row_reference;
        using worksheet_reference::col_reference;
        using worksheet_reference::cell_reference;
        struct cell {
            cell_reference ref;
            std::string raw;
            enum struct calculation_state_type { pending, in_progress, finished } calculation_state;
            bool needs_redraw = true;
            std::shared_ptr<const expression> expr;
            std::shared_ptr<const expression::primitive> value;
            cell(): ref(cell_reference(0, 0)), raw(""), expr(std::make_shared<const expression::text>("")), value(std::make_shared<expression::text>("")) {}
            cell(cell_reference ref, std::string raw, std::shared_ptr<expression::primitive> value): ref(ref), raw(raw), expr(value), value(value) {};
            
            std::shared_ptr<const expression::primitive> calculate() noexcept(0);
        };
    private:
        std::array<int, MAX_COL> col_width;
        std::array<int, MAX_ROW> row_height;
        std::array<int, MAX_ROW> row_start;
        std::array<int, MAX_ROW> col_start;
        int header_col_width = 3;
        int header_row_height = 2;
        const std::optional<terminal::rgb_color> border_color = {{50, 50, 50}};
        const std::optional<terminal::rgb_color> active_border_color = {{ 23, 88, 173 }};
        const std::optional<terminal::rgb_color> active_header_bg_color = {{ 75, 75, 75 }};
        const std::optional<terminal::rgb_color> active_header_fg_color = {{ 255, 0, 0 }};
        const std::optional<terminal::rgb_color> header_fg_color = {{ 255, 255, 255 }};
        const std::optional<terminal::rgb_color> header_bg_color = {};
    public:
        terminal::size bufsize;
        struct grid {
            struct grid_row {
                std::array<cell, MAX_COL> cells;

                cell& operator[](const col_reference& col) {
                    return cells[col.number];
                }
                cell& operator[](const int col) {
                    return cells[col];
                }
            };

            std::array<grid_row, MAX_ROW> cells;

            grid_row& operator[](const int row) {
                return cells[row];
            }
            grid_row& operator[](const row_reference& row) {
                return cells[row.number];
            }
            cell& operator[](const cell_reference& ref) {
                return (*this)[ref.row][ref.col];
            }
        };
        grid cells;
        cell_reference active_cell = cell_reference(0, 0);

        worksheet();

        void update_row_start();
        void update_col_start();

        void draw_row_lines();

        void draw_col_lines();

        void draw_header_row(col_reference col, bool is_active);

        void draw_header_col(row_reference row, bool is_active);

        void draw_cell_borders(const cell_reference& cell, const std::optional<terminal::rgb_color> bg);
        void draw_active_borders(const cell_reference& active_cell);

        void draw_cell_text(const cell_reference& cell);

        void redraw();

        void update_active_cell(const cell_reference& oldValue, const cell_reference& newValue);
        void update_needs_redraw_cell();

        void recalculate();
};

#endif
