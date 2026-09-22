#include "teacup/detail/renderer.hpp"

namespace teacup
{
namespace detail
{
namespace
{

std::string move_to(int x, int y)
{
    return "\x1b[" + std::to_string(y + 1) + ";" + std::to_string(x + 1) + "H";
}

}  // namespace

Renderer::Renderer(Terminal& out, ColorProfile profile) : out_{ out }, profile_{ profile } { }

void Renderer::invalidate()
{
    full_repaint_ = true;
    previous_ = Surface{};
}

std::string Renderer::build(const Surface& frame)
{
    const bool full = full_repaint_ || previous_.size() != frame.size();
    std::string out;
    if (full)
    {
        out += "\x1b[0m\x1b[2J";
    }

    Style active{};
    bool style_known = false;
    int cursor_x = -1;
    int cursor_y = -1;

    for (int y = 0; y < frame.height(); ++y)
    {
        for (int x = 0; x < frame.width(); ++x)
        {
            const auto p = Point{ x, y };
            const Cell& current = frame.at(p);
            if (current.width == 0)
            {
                continue;  // trailing half of a wide glyph
            }
            if (!full && previous_.contains(p) && previous_.at(p) == current)
            {
                continue;
            }
            if (cursor_x != x || cursor_y != y)
            {
                out += move_to(x, y);
            }
            if (!style_known || active != current.style)
            {
                out += sgr_sequence(current.style, profile_);
                active = current.style;
                style_known = true;
            }
            out += current.text.empty() ? " " : current.text;
            cursor_x = x + current.width;
            cursor_y = y;
        }
    }

    if (!out.empty())
    {
        out += "\x1b[0m";
    }
    previous_ = frame;
    full_repaint_ = false;
    return out;
}

void Renderer::draw(const Surface& frame)
{
    const std::string data = build(frame);
    if (data.empty())
    {
        return;
    }
    out_.write(data);
    out_.flush();
}

}  // namespace detail
}  // namespace teacup
