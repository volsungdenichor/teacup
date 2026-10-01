#include "teacup/surface.hpp"

namespace teacup
{
namespace
{
Cell& dummy_cell()
{
    static Cell value{};
    return value;
}
}  // namespace

Surface::Surface(Size s)
{
    resize(s);
}

void Surface::resize(Size s)
{
    size_.width = s.width > 0 ? s.width : 0;
    size_.height = s.height > 0 ? s.height : 0;
    cells_.assign(static_cast<std::size_t>(size_.area()), Cell{});
}

void Surface::clear(const Style& s)
{
    for (auto& c : cells_)
    {
        c = Cell{ Glyph{}, s };
    }
}

const Cell& Surface::at(const Point& p) const
{
    if (!contains(p))
    {
        return dummy_cell();
    }
    return cells_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)];
}

Cell& Surface::at(const Point& p)
{
    if (!contains(p))
    {
        dummy_cell() = Cell{};
        return dummy_cell();
    }
    return cells_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)];
}

std::string Surface::row_text(int y) const
{
    std::string out;
    for (int x = 0; x < size_.width; ++x)
    {
        const Cell& c = at(Point{ x, y });
        if (c.glyph.width() == 0)
        {
            continue;
        }
        out += c.glyph.utf8();
    }
    return out;
}

std::string to_plain_text(const Surface& s)
{
    std::string out;
    const int height = s.size().height;
    for (int y = 0; y < height; ++y)
    {
        if (y != 0)
        {
            out += '\n';
        }
        out += s.row_text(y);
    }
    return out;
}

}  // namespace teacup
