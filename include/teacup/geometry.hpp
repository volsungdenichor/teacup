#pragma once

#include <algorithm>

namespace teacup
{

struct point
{
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(const point& l, const point& r)
    {
        return l.x == r.x && l.y == r.y;
    }
    friend constexpr bool operator!=(const point& l, const point& r)
    {
        return !(l == r);
    }
};

struct size
{
    int width = 0;
    int height = 0;

    [[nodiscard]] constexpr int area() const
    {
        return width * height;
    }
    [[nodiscard]] constexpr bool empty() const
    {
        return width <= 0 || height <= 0;
    }

    friend constexpr bool operator==(const size& l, const size& r)
    {
        return l.width == r.width && l.height == r.height;
    }
    friend constexpr bool operator!=(const size& l, const size& r)
    {
        return !(l == r);
    }
};

struct rect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    static constexpr rect from_size(const teacup::size& s)
    {
        return rect{ 0, 0, s.width, s.height };
    }

    [[nodiscard]] constexpr int left() const
    {
        return x;
    }
    [[nodiscard]] constexpr int top() const
    {
        return y;
    }
    [[nodiscard]] constexpr int right() const
    {
        return x + width;
    }
    [[nodiscard]] constexpr int bottom() const
    {
        return y + height;
    }
    [[nodiscard]] constexpr teacup::size size() const
    {
        return { width, height };
    }
    [[nodiscard]] constexpr bool empty() const
    {
        return width <= 0 || height <= 0;
    }

    [[nodiscard]] constexpr bool contains(const point& p) const
    {
        return p.x >= left() && p.x < right() && p.y >= top() && p.y < bottom();
    }

    [[nodiscard]] rect intersect(const rect& other) const
    {
        const int nx = (std::max)(left(), other.left());
        const int ny = (std::max)(top(), other.top());
        const int nr = (std::min)(right(), other.right());
        const int nb = (std::min)(bottom(), other.bottom());
        if (nr <= nx || nb <= ny)
        {
            return rect{ nx, ny, 0, 0 };
        }
        return rect{ nx, ny, nr - nx, nb - ny };
    }

    /// Shrinks the rectangle by the given insets, never going below zero extent.
    [[nodiscard]] rect inset(int l, int t, int r, int b) const
    {
        const int nw = (std::max)(0, width - l - r);
        const int nh = (std::max)(0, height - t - b);
        return rect{ x + l, y + t, nw, nh };
    }

    [[nodiscard]] rect inset(int all) const
    {
        return inset(all, all, all, all);
    }

    [[nodiscard]] rect moved(int dx, int dy) const
    {
        return rect{ x + dx, y + dy, width, height };
    }

    friend constexpr bool operator==(const rect& l, const rect& r)
    {
        return l.x == r.x && l.y == r.y && l.width == r.width && l.height == r.height;
    }
    friend constexpr bool operator!=(const rect& l, const rect& r)
    {
        return !(l == r);
    }
};

enum class align
{
    start,
    center,
    end
};

/// Places `extent` inside `available` according to `a` and returns the offset.
constexpr int align_offset(align a, int available, int extent)
{
    switch (a)
    {
        case align::center: return (available - extent) / 2;
        case align::end: return available - extent;
        case align::start:
        default: return 0;
    }
}

}  // namespace teacup
