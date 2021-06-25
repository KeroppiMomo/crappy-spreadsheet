#ifndef __INCLUDE_WORKSPACE_
#define __INCLUDE_WORKSPACE_

#include "worksheet.h"

namespace workspace {
    enum struct mode_type: int;

    extern worksheet ws;
    extern mode_type mode;

    extern const std::optional<terminal::rgb_color> error_color;

    extern bool mark_flush;
    extern std::string insert_str;
    extern bool insert_parse_error;

    void render();

    bool isWordChar(char c);
    void action(char ch);
}

#endif
