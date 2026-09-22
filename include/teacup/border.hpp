#pragma once

#include <string>

namespace teacup
{

struct Border
{
    std::string top = "─";
    std::string bottom = "─";
    std::string left = "│";
    std::string right = "│";
    std::string top_left = "┌";
    std::string top_right = "┐";
    std::string bottom_left = "└";
    std::string bottom_right = "┘";

    static Border plain() { return {}; }

    static Border rounded()
    {
        Border b;
        b.top_left = "╭";
        b.top_right = "╮";
        b.bottom_left = "╰";
        b.bottom_right = "╯";
        return b;
    }

    static Border thick() { return { "━", "━", "┃", "┃", "┏", "┓", "┗", "┛" }; }

    static Border double_line() { return { "═", "═", "║", "║", "╔", "╗", "╚", "╝" }; }

    static Border ascii() { return { "-", "-", "|", "|", "+", "+", "+", "+" }; }

    static Border blank() { return { " ", " ", " ", " ", " ", " ", " ", " " }; }
};

}  // namespace teacup
