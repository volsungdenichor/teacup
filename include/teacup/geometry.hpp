#pragma once

#include <algorithm>

namespace teacup
{

struct Point
{
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(const Point& lhs, const Point& rhs) { return lhs.x == rhs.x && lhs.y == rhs.y; }
    friend constexpr bool operator!=(const Point& lhs, const Point& rhs) { return !(lhs == rhs); }

    friend constexpr Point operator+(const Point& lhs, const Point& rhs) { return Point{ lhs.x + rhs.x, lhs.y + rhs.y }; }
    friend constexpr Point operator-(const Point& lhs, const Point& rhs) { return Point{ lhs.x - rhs.x, lhs.y - rhs.y }; }
};

struct Size
{
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr int area() const { return width * height; }
    [[nodiscard]] constexpr bool empty() const { return width <= 0 || height <= 0; }

    friend constexpr bool operator==(const Size& lhs, const Size& rhs)
    {
        return lhs.width == rhs.width && lhs.height == rhs.height;
    }
    friend constexpr bool operator!=(const Size& lhs, const Size& rhs) { return !(lhs == rhs); }
};

struct Rect
{
    Point _origin{ 0, 0 };
    Size _size{ 0, 0 };

    constexpr Rect() = default;
    constexpr Rect(Point origin, Size size) : _origin(origin), _size(size) { }
    constexpr Rect(int x, int y, int width, int height) : _origin{ x, y }, _size{ width, height } { }

    static constexpr Rect from_size(const Size& s) { return Rect{ Point{ 0, 0 }, s }; }

    [[nodiscard]] constexpr Size size() const { return _size; }
    [[nodiscard]] constexpr Point origin() const { return _origin; }

    [[nodiscard]] constexpr int left() const { return _origin.x; }
    [[nodiscard]] constexpr int top() const { return _origin.y; }
    [[nodiscard]] constexpr int right() const { return _origin.x + _size.width; }
    [[nodiscard]] constexpr int bottom() const { return _origin.y + _size.height; }
    [[nodiscard]] constexpr int width() const { return _size.width; }
    [[nodiscard]] constexpr int height() const { return _size.height; }
    [[nodiscard]] constexpr bool empty() const { return _size.empty(); }

    [[nodiscard]] constexpr bool contains(const Point& p) const
    {
        return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
    }

    [[nodiscard]] Rect intersect(const Rect& other) const
    {
        const int nx = (std::max)(left(), other.left());
        const int ny = (std::max)(top(), other.top());
        const int nr = (std::min)(right(), other.right());
        const int nb = (std::min)(bottom(), other.bottom());
        if (nr <= nx || nb <= ny)
        {
            return Rect{ Point{ nx, ny }, Size{ 0, 0 } };
        }
        return Rect{ Point{ nx, ny }, Size{ nr - nx, nb - ny } };
    }

    /// Shrinks the rectangle by the given insets, never going below zero extent.
    [[nodiscard]] Rect inset(int l, int t, int r, int b) const
    {
        const int nw = (std::max)(0, width() - l - r);
        const int nh = (std::max)(0, height() - t - b);
        return Rect{ _origin.x + l, _origin.y + t, nw, nh };
    }

    [[nodiscard]] Rect inset(int all) const { return inset(all, all, all, all); }

    [[nodiscard]] Rect moved(const Point& offset) const { return Rect{ _origin + offset, _size }; }

    friend constexpr bool operator==(const Rect& lhs, const Rect& rhs)
    {
        return lhs._origin == rhs._origin && lhs._size == rhs._size;
    }

    friend constexpr bool operator!=(const Rect& lhs, const Rect& rhs) { return !(lhs == rhs); }

    template <class Func>
    void for_each(const Func& f) const
    {
        for (int y = top(); y < bottom(); ++y)
        {
            for (int x = left(); x < right(); ++x)
            {
                f(Point{ x, y });
            }
        }
    }
};

enum class Align
{
    start,
    center,
    end
};

/// Places `extent` inside `available` according to `a` and returns the offset.
constexpr int align_offset(Align a, int available, int extent)
{
    switch (a)
    {
        case Align::center: return (available - extent) / 2;
        case Align::end: return available - extent;
        case Align::start:
        default: return 0;
    }
}

}  // namespace teacup
