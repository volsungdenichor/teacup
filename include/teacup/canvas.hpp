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

    void put(const Point& p, const Cell& c);

    void style_at(const Point& p, const Style& s);

    int text(const Point& p, const std::string& value, const Style& s = {});

    int text_aligned(int y, const std::string& value, Align a, const Style& s = {});

    void fill(const Rect& area, const std::string& glyph = " ", const Style& s = {});
    void fill(const std::string& glyph = " ", const Style& s = {}) { fill(bounds(), glyph, s); }

    void horizontal_line(const Point& p, int length, const std::string& glyph, const Style& s = {});
    void vertical_line(const Point& p, int length, const std::string& glyph, const Style& s = {});

private:
    Surface* target_ = nullptr;
    Rect area_{};
};

}  // namespace teacup
