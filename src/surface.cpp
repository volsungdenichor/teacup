#include "teacup/surface.hpp"

namespace teacup
{
namespace
{
cell& dummy_cell()
{
    static cell value{};
    return value;
}
}  // namespace

surface::surface(teacup::size s)
{
    resize(s);
}

void surface::resize(teacup::size s)
{
    size_.width = s.width > 0 ? s.width : 0;
    size_.height = s.height > 0 ? s.height : 0;
    cells_.assign(static_cast<std::size_t>(size_.area()), cell{});
}

void surface::clear(const teacup::style& s)
{
    for (auto& c : cells_)
    {
        c = cell{ " ", s, 1 };
    }
}

const cell& surface::at(const point& p) const
{
    if (!contains(p))
    {
        return dummy_cell();
    }
    return cells_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)];
}

cell& surface::at(const point& p)
{
    if (!contains(p))
    {
        dummy_cell() = cell{};
        return dummy_cell();
    }
    return cells_[static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) + static_cast<std::size_t>(p.x)];
}

std::string surface::row_text(int y) const
{
    std::string out;
    for (int x = 0; x < size_.width; ++x)
    {
        const cell& c = at(point{ x, y });
        if (c.width == 0)
        {
            continue;
        }
        out += c.text;
    }
    return out;
}

std::string to_plain_text(const surface& s)
{
    std::string out;
    for (int y = 0; y < s.height(); ++y)
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
