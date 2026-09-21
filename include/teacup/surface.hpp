#pragma once

#include <string>
#include <vector>

#include "teacup/geometry.hpp"
#include "teacup/style.hpp"

namespace teacup
{

/// One terminal cell: a UTF-8 grapheme plus its appearance.
struct cell
{
    std::string text = " ";
    teacup::style style{};
    /// Columns occupied: 1 normally, 2 for wide glyphs, 0 for the trailing half.
    std::uint8_t width = 1;

    friend bool operator==(const cell& lhs, const cell& rhs)
    {
        return lhs.width == rhs.width && lhs.text == rhs.text && lhs.style == rhs.style;
    }
    friend bool operator!=(const cell& lhs, const cell& rhs) { return !(lhs == rhs); }
};

/// An owning grid of cells; the render target of every widget.
class surface
{
public:
    surface() = default;
    explicit surface(teacup::size s);

    [[nodiscard]] teacup::size size() const { return size_; }
    [[nodiscard]] int width() const { return size_.width; }
    [[nodiscard]] int height() const { return size_.height; }
    [[nodiscard]] rect bounds() const { return rect::from_size(size_); }

    void resize(teacup::size s);
    void clear(const teacup::style& s = {});

    [[nodiscard]] bool contains(const point& p) const
    {
        return p.x >= 0 && p.y >= 0 && p.x < size_.width && p.y < size_.height;
    }

    [[nodiscard]] const cell& at(const point& p) const;
    cell& at(const point& p);

    /// Text content of one row, ignoring styles. Handy in tests.
    [[nodiscard]] std::string row_text(int y) const;

private:
    teacup::size size_{};
    std::vector<cell> cells_{};
};

/// Plain-text dump of the whole surface, rows separated by '\n'.
std::string to_plain_text(const surface& s);

}  // namespace teacup
