#include "teacup/detail/renderer.hpp"

namespace teacup
{
namespace detail
{
namespace
{

std::string move_to(const Point& p)
{
    return "\x1b[" + std::to_string(p.y + 1) + ";" + std::to_string(p.x + 1) + "H";
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
    Point cursor{ -1, -1 };

    frame.bounds().for_each(
        [&](const Point& p)
        {
            const Cell& current = frame.at(p);
            if (current.glyph.width() == 0)
            {
                return;  // trailing half of a wide glyph
            }
            if (!full && previous_.contains(p) && previous_.at(p) == current)
            {
                return;
            }
            if (cursor != p)
            {
                out += move_to(p);
            }
            if (!style_known || active != current.style)
            {
                out += sgr_sequence(current.style, profile_);
                active = current.style;
                style_known = true;
            }
            out += current.glyph.utf8();
            cursor = Point{ p.x + current.glyph.width(), p.y };
        });

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
