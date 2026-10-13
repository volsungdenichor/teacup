#include "teacup/canvas.hpp"

namespace teacup
{

Canvas::Canvas(Surface& target, Rect area) : target_{ &target }, area_{ area.intersect(target.bounds()) } { }

Canvas Canvas::sub(const Rect& area) const
{
    return sub_absolute(area.moved(area_.origin()));
}

Canvas Canvas::sub_absolute(const Rect& area) const
{
    return Canvas{ *target_, area.intersect(area_) };
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

int Canvas::text(const Point& p, const GlyphString& value, const Style& s)
{
    if (p.y < 0 || p.y >= area_.height())
    {
        return 0;
    }
    int cursor = p.x;
    for (const Glyph& glyph : value)
    {
        if (glyph.utf8() == "\n" || glyph.utf8() == "\r\n" || !place(cursor, p.y, glyph, s))
        {
            break;
        }
    }
    return cursor - p.x;
}

bool Canvas::place(int& x, int y, const Glyph& glyph, const Style& s)
{
    const int w = glyph.width();
    if (w == 0)
    {
        return true;
    }
    if (x + w > area_.width())
    {
        return false;
    }
    if (x >= 0)
    {
        put(Point{ x, y }, Cell{ glyph, s });
        if (w == 2)
        {
            put(Point{ x + 1, y }, Cell{ Glyph::continuation(), s });
        }
    }
    x += w;
    return true;
}

int Canvas::text_aligned(int y, const GlyphString& value, Align a, const Style& s)
{
    return text(Point{ align_offset(a, area_.width(), value.width()), y }, value, s);
}

void Canvas::fill(const Rect& area, const Glyph& glyph, const Style& s)
{
    const Rect local = area.intersect(bounds());
    const Glyph cell_glyph = glyph.width() == 0 ? Glyph{} : glyph;
    const int w = cell_glyph.width();
    for (int y = local.top(); y < local.bottom(); ++y)
    {
        for (int x = local.left(); x < local.right(); x += w)
        {
            put(Point{ x, y }, Cell{ cell_glyph, s });
            for (int k = 1; k < w && x + k < local.right(); ++k)
            {
                put(Point{ x + k, y }, Cell{ Glyph::continuation(), s });
            }
        }
    }
}

void Canvas::horizontal_line(const Point& p, int length, const Glyph& glyph, const Style& s)
{
    fill(Rect{ p, Size{ length, 1 } }, glyph, s);
}

void Canvas::vertical_line(const Point& p, int length, const Glyph& glyph, const Style& s)
{
    fill(Rect{ p, Size{ 1, length } }, glyph, s);
}

}  // namespace teacup
