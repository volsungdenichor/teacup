#pragma once

#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace widgets
{

/// Splits `text` into lines no wider than `width` columns, breaking on spaces
/// when possible and honouring embedded newlines.
std::vector<std::string> wrap_text(const std::string& text, int width);

/// A block of styled, optionally wrapped and aligned text.
class text
{
public:
    text() = default;
    explicit text(std::string content)
        : content_{ std::move(content) }
    {
    }

    text& content(std::string value) &
    {
        content_ = std::move(value);
        return *this;
    }
    text&& content(std::string value) &&
    {
        return std::move(this->content(std::move(value)));
    }

    text& style(const teacup::style& s) &
    {
        style_ = s;
        return *this;
    }
    text&& style(const teacup::style& s) &&
    {
        return std::move(this->style(s));
    }

    text& align(teacup::align a) &
    {
        align_ = a;
        return *this;
    }
    text&& align(teacup::align a) &&
    {
        return std::move(this->align(a));
    }

    text& vertical_align(teacup::align a) &
    {
        valign_ = a;
        return *this;
    }
    text&& vertical_align(teacup::align a) &&
    {
        return std::move(this->vertical_align(a));
    }

    text& wrap(bool enabled) &
    {
        wrap_ = enabled;
        return *this;
    }
    text&& wrap(bool enabled) &&
    {
        return std::move(this->wrap(enabled));
    }

    /// Number of rows the text needs at the given width.
    [[nodiscard]] int height_for(int width) const
    {
        return static_cast<int>(lines(width).size());
    }

    void render(canvas target) const
    {
        if (!target.valid())
        {
            return;
        }
        const std::vector<std::string> rows = lines(target.width());
        const int offset = align_offset(
            valign_, target.height(), static_cast<int>(rows.size()));
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            const int y = offset + static_cast<int>(i);
            if (y < 0 || y >= target.height())
            {
                continue;
            }
            target.text_aligned(y, rows[i], align_, style_);
        }
    }

private:
    [[nodiscard]] std::vector<std::string> lines(int width) const
    {
        if (wrap_)
        {
            return wrap_text(content_, width);
        }
        return wrap_text(content_, 0);
    }

    std::string content_{};
    teacup::style style_{};
    teacup::align align_ = teacup::align::start;
    teacup::align valign_ = teacup::align::start;
    bool wrap_ = true;
};

}  // namespace widgets
}  // namespace teacup
