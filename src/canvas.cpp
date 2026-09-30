#include "teacup/canvas.hpp"

#include "teacup/detail/utf8.hpp"

namespace teacup
{

Canvas::Canvas(Surface& target, Rect area) : target_{ &target }, area_{ area.intersect(target.bounds()) } { }

Canvas Canvas::sub(const Rect& area) const
{
    return Canvas{ *target_, area.moved(area_.origin()).intersect(area_) };
}

Canvas Canvas::horizontal_slice(int y, int height) const
{
    return sub(Rect{ Point{ 0, y }, Size{ size().width, height } });
}

Canvas Canvas::indent(int x) const
{
    return sub(Rect{ Point{ x, 0 }, Size{ size().width - x, size().height } });
}

void Canvas::put(const Point& p, const Cell& c)
{
    if (bounds().contains(p))
    {
        target_->at(area_.origin() + p) = c;
    }
}

void Canvas::style_at(const Point& p, const Style& s)
{
    if (bounds().contains(p))
    {
        target_->at(area_.origin() + p).style |= s;
    }
}

int Canvas::text(const Point& p, const std::string& value, const Style& s)
{
    if (p.y < 0 || p.y >= area_.height())
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
            put(Point{ cursor, p.y }, Cell{ glyph, s, static_cast<std::uint8_t>(w) });
            if (w == 2 && cursor + 1 < area_.width())
            {
                put(Point{ cursor + 1, p.y }, Cell{ "", s, 0 });
            }
        }
        cursor += w;
    }
    return cursor - p.x;
}

int Canvas::text_aligned(int y, const std::string& value, Align a, const Style& s)
{
    const int extent = utf8::width(value);
    return text(Point{ align_offset(a, area_.width(), extent), y }, value, s);
}

void Canvas::fill(const Rect& area, const std::string& glyph, const Style& s)
{
    const Rect local = area.intersect(bounds());
    const int w = glyph.empty() ? 1 : utf8::width(glyph);
    for (int y = local.top(); y < local.bottom(); ++y)
    {
        for (int x = local.left(); x < local.right(); x += w)
        {
            put(Point{ x, y }, Cell{ glyph, s, static_cast<std::uint8_t>(w) });
            for (int k = 1; k < w && x + k < local.right(); ++k)
            {
                put(Point{ x + k, y }, Cell{ "", s, 0 });
            }
        }
    }
}

void Canvas::horizontal_line(const Point& p, int length, const std::string& glyph, const Style& s)
{
    fill(Rect{ p, Size{ length, 1 } }, glyph, s);
}

void Canvas::vertical_line(const Point& p, int length, const std::string& glyph, const Style& s)
{
    fill(Rect{ p, Size{ 1, length } }, glyph, s);
}

}  // namespace teacup
