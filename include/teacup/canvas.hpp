#pragma once

#include <string>

#include "teacup/surface.hpp"

namespace teacup

{

class canvas
{
public:
    canvas() = default;
    canvas(surface& target, rect area);

    explicit canvas(surface& target) : canvas{ target, target.bounds() } { }

    [[nodiscard]] teacup::size size() const { return area_.size(); }
    [[nodiscard]] int width() const { return size().width; }
    [[nodiscard]] int height() const { return size().height; }
    [[nodiscard]] rect bounds() const { return rect::from_size(size()); }
    [[nodiscard]] bool valid() const { return target_ != nullptr && !area_.empty(); }

    [[nodiscard]] canvas sub(const rect& area) const;

    void put(const point& p, const cell& c);

    void style_at(const point& p, const style& s);

    int text(const point& p, const std::string& value, const style& s = {});

    int text_aligned(int y, const std::string& value, align a, const style& s = {});

    void fill(const rect& area, const std::string& glyph = " ", const style& s = {});
    void fill(const std::string& glyph = " ", const style& s = {}) { fill(bounds(), glyph, s); }

    void horizontal_line(const point& p, int length, const std::string& glyph, const style& s = {});
    void vertical_line(const point& p, int length, const std::string& glyph, const style& s = {});

private:
    surface* target_ = nullptr;
    rect area_{};
};

}  // namespace teacup
