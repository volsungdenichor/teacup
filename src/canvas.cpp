#include "teacup/canvas.hpp"

#include "teacup/detail/utf8.hpp"

namespace teacup
{

canvas::canvas(surface& target, rect area) : target_{ &target }, area_{ area.intersect(target.bounds()) } { }

canvas canvas::sub(const rect& area) const
{
    canvas result;
    if (target_ == nullptr)
    {
        return result;
    }
    result.target_ = target_;
    result.area_ = area.moved(area_.origin()).intersect(area_);
    return result;
}

void canvas::put(const point& p, const cell& c)
{
    if (target_ == nullptr || !bounds().contains(p))
    {
        return;
    }
    target_->at(area_.origin() + p) = c;
}

void canvas::style_at(const point& p, const style& s)
{
    if (target_ == nullptr || !bounds().contains(p))
    {
        return;
    }
    cell& target = target_->at(area_.origin() + p);
    target.style = target.style.merged(s);
}

int canvas::text(const point& p, const std::string& value, const style& s)
{
    if (target_ == nullptr || p.y < 0 || p.y >= area_.height())
    {
        return 0;
    }
    int cursor = p.x;
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
        if (cursor >= area_.width())
        {
            break;
        }
        std::string glyph;
        utf8::encode(cp, glyph);
        if (cursor >= 0)
        {
            put(point{ cursor, p.y }, cell{ glyph, s, static_cast<std::uint8_t>(w) });
            if (w == 2 && cursor + 1 < area_.width())
            {
                put(point{ cursor + 1, p.y }, cell{ "", s, 0 });
            }
        }
        cursor += w;
    }
    return cursor - p.x;
}

int canvas::text_aligned(int y, const std::string& value, align a, const style& s)
{
    const int extent = utf8::width(value);
    return text(point{ align_offset(a, area_.width(), extent), y }, value, s);
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
            put(point{ x, y }, cell{ glyph, s, static_cast<std::uint8_t>(step) });
            for (int k = 1; k < step && x + k < local.right(); ++k)
            {
                put(point{ x + k, y }, cell{ "", s, 0 });
            }
        }
    }
}

void canvas::horizontal_line(const point& p, int length, const std::string& glyph, const style& s)
{
    fill(rect{ p, teacup::size{ length, 1 } }, glyph, s);
}

void canvas::vertical_line(const point& p, int length, const std::string& glyph, const style& s)
{
    fill(rect{ p, teacup::size{ 1, length } }, glyph, s);
}

}  // namespace teacup
