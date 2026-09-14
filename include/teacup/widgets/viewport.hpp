#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

/// A vertically scrollable collection of clipped text lines.
class viewport
{
public:
    viewport() = default;

    viewport& lines(std::vector<std::string> value) &
    {
        lines_ = std::move(value);
        clamp_offset();
        return *this;
    }
    viewport&& lines(std::vector<std::string> value) &&
    {
        return std::move(this->lines(std::move(value)));
    }

    viewport& offset(int value) &
    {
        offset_ = value;
        clamp_offset();
        return *this;
    }
    viewport&& offset(int value) &&
    {
        return std::move(this->offset(value));
    }

    viewport& style(const teacup::style& value) &
    {
        style_ = value;
        return *this;
    }
    viewport&& style(const teacup::style& value) &&
    {
        return std::move(this->style(value));
    }

    viewport& scroll(int amount) &
    {
        offset_ += amount;
        clamp_offset();
        return *this;
    }
    viewport&& scroll(int amount) &&
    {
        return std::move(this->scroll(amount));
    }

    viewport& page_down(int height) &
    {
        return scroll((std::max)(1, height));
    }
    viewport&& page_down(int height) &&
    {
        return std::move(this->page_down(height));
    }

    viewport& page_up(int height) &
    {
        return scroll(-((std::max)(1, height)));
    }
    viewport&& page_up(int height) &&
    {
        return std::move(this->page_up(height));
    }

    [[nodiscard]] int offset() const
    {
        return offset_;
    }

    [[nodiscard]] int max_offset(int height) const
    {
        return (std::max)(0, static_cast<int>(lines_.size()) - (std::max)(0, height));
    }

    void render(canvas target) const
    {
        if (!target.valid())
        {
            return;
        }

        const int width = target.width();
        const int height = target.height();
        if (width <= 0 || height <= 0)
        {
            return;
        }

        const int first = (std::min)(offset_, max_offset(height));
        for (int row = 0; row < height; ++row)
        {
            target.fill(rect{ 0, row, width, 1 }, " ");
            const int line_index = first + row;
            if (line_index < static_cast<int>(lines_.size()))
            {
                target.text(0, row, lines_[static_cast<std::size_t>(line_index)], style_);
            }
        }
    }

private:
    void clamp_offset()
    {
        offset_ = (std::max)(0, offset_);
        offset_ = (std::min)(offset_, static_cast<int>(lines_.size()));
    }

    std::vector<std::string> lines_;
    int offset_ = 0;
    teacup::style style_;
};

}  // namespace widgets
}  // namespace teacup