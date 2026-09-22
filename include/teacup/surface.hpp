#pragma once

#include <string>
#include <vector>

#include "teacup/geometry.hpp"
#include "teacup/style.hpp"

namespace teacup
{

struct Cell
{
    std::string text = " ";
    Style style{};
    std::uint8_t width = 1;

    friend bool operator==(const Cell& lhs, const Cell& rhs)
    {
        return lhs.width == rhs.width && lhs.text == rhs.text && lhs.style == rhs.style;
    }

    friend bool operator!=(const Cell& lhs, const Cell& rhs) { return !(lhs == rhs); }
};
class Surface
{
public:
    Surface() = default;
    explicit Surface(Size s);

    [[nodiscard]] Size size() const { return size_; }
    [[nodiscard]] int width() const { return size_.width; }
    [[nodiscard]] int height() const { return size_.height; }
    [[nodiscard]] Rect bounds() const { return Rect::from_size(size_); }

    void resize(Size s);
    void clear(const Style& s = {});

    [[nodiscard]] bool contains(const Point& p) const
    {
        return p.x >= 0 && p.y >= 0 && p.x < size_.width && p.y < size_.height;
    }

    [[nodiscard]] const Cell& at(const Point& p) const;
    Cell& at(const Point& p);

    [[nodiscard]] std::string row_text(int y) const;

private:
    Size size_{};
    std::vector<Cell> cells_{};
};

std::string to_plain_text(const Surface& s);

}  // namespace teacup
