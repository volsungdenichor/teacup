#pragma once

#include <string>

namespace teacup
{

struct border
{
    std::string top = "─";
    std::string bottom = "─";
    std::string left = "│";
    std::string right = "│";
    std::string top_left = "┌";
    std::string top_right = "┐";
    std::string bottom_left = "└";
    std::string bottom_right = "┘";

    static border plain() { return {}; }

    static border rounded()
    {
        border b;
        b.top_left = "╭";
        b.top_right = "╮";
        b.bottom_left = "╰";
        b.bottom_right = "╯";
        return b;
    }

    static border thick() { return { "━", "━", "┃", "┃", "┏", "┓", "┗", "┛" }; }

    static border double_line() { return { "═", "═", "║", "║", "╔", "╗", "╚", "╝" }; }

    static border ascii() { return { "-", "-", "|", "|", "+", "+", "+", "+" }; }

    static border blank() { return { " ", " ", " ", " ", " ", " ", " ", " " }; }
};

}  // namespace teacup
