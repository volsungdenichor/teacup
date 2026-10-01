#pragma once

#include <string>
#include <vector>

#include "teacup/geometry.hpp"
#include "teacup/glyph.hpp"
#include "teacup/style.hpp"

namespace teacup
{

struct Cell
{
    Glyph glyph{};
    Style style{};

    friend bool operator==(const Cell& lhs, const Cell& rhs) { return lhs.glyph == rhs.glyph && lhs.style == rhs.style; }

    friend bool operator!=(const Cell& lhs, const Cell& rhs) { return !(lhs == rhs); }
};

class Surface
{
public:
    Surface() = default;
    explicit Surface(Size s);

    [[nodiscard]] Size size() const { return size_; }
    [[nodiscard]] Rect bounds() const { return Rect::from_size(size_); }

    void resize(Size s);
    void clear(const Style& s = {});

    [[nodiscard]] bool contains(const Point& p) const { return bounds().contains(p); }

    [[nodiscard]] const Cell& at(const Point& p) const;
    Cell& at(const Point& p);

    [[nodiscard]] std::string row_text(int y) const;

private:
    Size size_{};
    std::vector<Cell> cells_{};
};

std::string to_plain_text(const Surface& s);

}  // namespace teacup
