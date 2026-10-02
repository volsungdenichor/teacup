#pragma once

#include "teacup/glyph.hpp"

namespace teacup
{

struct Border
{
    Glyph top = "─";
    Glyph bottom = "─";
    Glyph left = "│";
    Glyph right = "│";
    Glyph top_left = "┌";
    Glyph top_right = "┐";
    Glyph bottom_left = "└";
    Glyph bottom_right = "┘";

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
