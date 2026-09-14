#pragma once

#include <string>

#include "teacup/surface.hpp"

namespace teacup

{

/// A clipped, translated view of a `surface`.
///
/// Widgets always draw through a canvas, so coordinates are local (0,0 is the
/// top-left of the widget area) and out-of-bounds writes are silently dropped.
/// Canvases are cheap values; `sub()` produces a narrower one for children.
class canvas
{
public:
    canvas() = default;
    canvas(surface& target, rect area);

    explicit canvas(surface& target)
        : canvas{ target, target.bounds() }
    {
    }

    [[nodiscard]] teacup::size size() const
    {
        return area_.size();
    }
    [[nodiscard]] int width() const
    {
        return area_.width;
    }
    [[nodiscard]] int height() const
    {
        return area_.height;
    }
    /// Local bounds of this canvas, i.e. `{0, 0, width, height}`.
    [[nodiscard]] rect bounds() const
    {
        return rect::from_size(area_.size());
    }
    [[nodiscard]] bool valid() const
    {
        return target_ != nullptr && !area_.empty();
    }

    /// Narrows the canvas to `area` expressed in local coordinates.
    [[nodiscard]] canvas sub(const rect& area) const;

    void put(int x, int y, const cell& c);
    void put(const point& p, const cell& c)
    {
        put(p.x, p.y, c);
    }

    /// Applies `s` on top of the existing style of a cell.
    void style_at(int x, int y, const style& s);

    /// Draws `text` starting at (x, y), clipped to the canvas. Returns the
    /// number of columns advanced.
    int text(int x, int y, const std::string& text, const style& s = {});
    int text(const point& p, const std::string& value, const style& s = {})
    {
        return text(p.x, p.y, value, s);
    }

    /// Draws `text` horizontally aligned within the full canvas width.
    int text_aligned(int y, const std::string& value, align a, const style& s = {});

    /// Repeats `glyph` over `area` (local coordinates).
    void fill(const rect& area, const std::string& glyph = " ", const style& s = {});
    void fill(const std::string& glyph = " ", const style& s = {})
    {
        fill(bounds(), glyph, s);
    }

    void horizontal_line(int x, int y, int length, const std::string& glyph, const style& s = {});
    void vertical_line(int x, int y, int length, const std::string& glyph, const style& s = {});

private:
    surface* target_ = nullptr;
    /// Absolute area inside the target surface.
    rect area_{};
};

}  // namespace teacup
