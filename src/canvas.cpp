#include "teacup/canvas.hpp"

#include "teacup/detail/utf8.hpp"

namespace teacup
{

canvas::canvas(surface& target, rect area)
    : target_{ &target }
    , area_{ area.intersect(target.bounds()) }
{
}

canvas canvas::sub(const rect& area) const
{
    canvas result;
    if (target_ == nullptr)
    {
        return result;
    }
    result.target_ = target_;
    result.area_ = area.moved(area_.x, area_.y).intersect(area_);
    return result;
}

void canvas::put(int x, int y, const cell& c)
{
    if (target_ == nullptr || x < 0 || y < 0 || x >= area_.width || y >= area_.height)
    {
        return;
    }
    target_->at(area_.x + x, area_.y + y) = c;
}

void canvas::style_at(int x, int y, const style& s)
{
    if (target_ == nullptr || x < 0 || y < 0 || x >= area_.width || y >= area_.height)
    {
        return;
    }
    cell& target = target_->at(area_.x + x, area_.y + y);
    target.style = target.style.merged(s);
}

int canvas::text(int x, int y, const std::string& value, const style& s)
{
    if (target_ == nullptr || y < 0 || y >= area_.height)
    {
        return 0;
    }
    int cursor = x;
    std::size_t pos = 0;
    while (pos < value.size())
    {
        const char32_t cp = utf8::decode(value, pos);
        if (cp == U'\n')
        {
            break;
        }
        const int w = utf8::width(cp);
        if (w == 0)
        {
            continue;
        }
        if (cursor >= area_.width)
        {
            break;
        }
        std::string glyph;
        utf8::encode(cp, glyph);
        if (cursor >= 0)
        {
            put(cursor, y, cell{ glyph, s, static_cast<std::uint8_t>(w) });
            if (w == 2 && cursor + 1 < area_.width)
            {
                put(cursor + 1, y, cell{ "", s, 0 });
            }
        }
        cursor += w;
    }
    return cursor - x;
}

int canvas::text_aligned(int y, const std::string& value, align a, const style& s)
{
    const int extent = utf8::width(value);
    return text(align_offset(a, area_.width, extent), y, value, s);
}

void canvas::fill(const rect& area, const std::string& glyph, const style& s)
{
    const rect local = area.intersect(bounds());
    const int w = glyph.empty() ? 1 : utf8::width(glyph);
    const auto step = w <= 0 ? 1 : w;
    for (int y = local.top(); y < local.bottom(); ++y)
    {
        for (int x = local.left(); x < local.right(); x += step)
        {
            put(x, y, cell{ glyph, s, static_cast<std::uint8_t>(step) });
            for (int k = 1; k < step && x + k < local.right(); ++k)
            {
                put(x + k, y, cell{ "", s, 0 });
            }
        }
    }
}

void canvas::horizontal_line(int x, int y, int length, const std::string& glyph, const style& s)
{
    fill(rect{ x, y, length, 1 }, glyph, s);
}

void canvas::vertical_line(int x, int y, int length, const std::string& glyph, const style& s)
{
    fill(rect{ x, y, 1, length }, glyph, s);
}

}  // namespace teacup
