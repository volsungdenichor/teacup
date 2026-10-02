#pragma once

#include <string>

#include "teacup/surface.hpp"

namespace teacup

{

class Canvas
{
public:
    Canvas(Surface& target, Rect area);

    explicit Canvas(Surface& target) : Canvas{ target, target.bounds() } { }

    [[nodiscard]] Size size() const { return area_.size(); }
    [[nodiscard]] Rect bounds() const { return Rect::from_size(size()); }
    [[nodiscard]] bool valid() const { return !area_.empty(); }

    [[nodiscard]] Canvas sub(const Rect& area) const;
    [[nodiscard]] Canvas horizontal_slice(int y, int height = 1) const;
    [[nodiscard]] Canvas indent(int x) const;

    void put(const Point& p, const Cell& c);

    void style_at(const Point& p, const Style& s);

    int text(const Point& p, const GlyphString& value, const Style& s = {});

    int text_aligned(int y, const GlyphString& value, Align a, const Style& s = {});

    void fill(const Rect& area, const Glyph& glyph = Glyph{}, const Style& s = {});
    void fill(const Glyph& glyph = Glyph{}, const Style& s = {}) { fill(bounds(), glyph, s); }

    void horizontal_line(const Point& p, int length, const Glyph& glyph, const Style& s = {});
    void vertical_line(const Point& p, int length, const Glyph& glyph, const Style& s = {});

private:
    // Writes `glyph` at column `x` of row `y` and advances `x`; false once the row is full.
    bool place(int& x, int y, const Glyph& glyph, const Style& s);

    Surface* target_ = nullptr;
    Rect area_{};
};

}  // namespace teacup
