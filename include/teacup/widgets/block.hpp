#pragma once

#include <string>
#include <utility>

#include "teacup/border.hpp"
#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace widgets
{

/// A bordered container with an optional title and inner padding.
///
/// `render` draws the frame and hands the inner canvas to a callback, which
/// keeps composition explicit and free of ownership questions.
class block
{
public:
    block& border(const teacup::border& b)
    {
        border_ = b;
        has_border_ = true;
        return *this;
    }
    block& border_style(const style& s)
    {
        border_style_ = s;
        return *this;
    }
    block& title(std::string text)
    {
        title_ = std::move(text);
        return *this;
    }
    block& title_style(const style& s)
    {
        title_style_ = s;
        return *this;
    }
    block& title_align(align a)
    {
        title_align_ = a;
        return *this;
    }
    block& padding(int all)
    {
        return padding(all, all, all, all);
    }
    block& padding(int left, int top, int right, int bottom)
    {
        pad_left_ = left;
        pad_top_ = top;
        pad_right_ = right;
        pad_bottom_ = bottom;
        return *this;
    }
    block& background(const color& c)
    {
        background_ = c;
        return *this;
    }
    /// The area left for content once border and padding are subtracted.
    [[nodiscard]] rect inner(const rect& area) const
    {
        const int edge = has_border_ ? 1 : 0;
        return area.inset(edge + pad_left_, edge + pad_top_, edge + pad_right_, edge + pad_bottom_);
    }

    /// Draws the frame and returns the canvas for the content.
    canvas draw(canvas target) const
    {
        if (background_.is_set())
        {
            target.fill(" ", style{}.bg(background_));
        }
        if (has_border_)
        {
            draw_border(target);
        }
        return target.sub(inner(target.bounds()));
    }

    template <typename Fn>
    void render(canvas target, Fn&& content) const
    {
        canvas inner_canvas = draw(target);
        if (inner_canvas.valid())
        {
            std::forward<Fn>(content)(inner_canvas);
        }
    }

    void render(canvas target) const
    {
        draw(target);
    }

private:
    void draw_border(canvas& target) const
    {
        const int w = target.width();
        const int h = target.height();
        if (w <= 0 || h <= 0)
        {
            return;
        }
        const style& s = border_style_;

        if (h == 1)
        {
            target.horizontal_line(0, 0, w, border_.top, s);
        }
        else
        {
            target.horizontal_line(1, 0, w - 2, border_.top, s);
            target.horizontal_line(1, h - 1, w - 2, border_.bottom, s);
            target.vertical_line(0, 1, h - 2, border_.left, s);
            target.vertical_line(w - 1, 1, h - 2, border_.right, s);
            target.put(0, 0, cell{ border_.top_left, s, 1 });
            target.put(w - 1, 0, cell{ border_.top_right, s, 1 });
            target.put(0, h - 1, cell{ border_.bottom_left, s, 1 });
            target.put(w - 1, h - 1, cell{ border_.bottom_right, s, 1 });
        }

        if (!title_.empty() && w > 2)
        {
            const int available = w - 2;
            const int extent = (std::min)(utf8::width(title_), available);
            const int offset = 1 + align_offset(title_align_, available, extent);
            target.text(offset, 0, title_, title_style_);
        }
    }

    teacup::border border_{};
    bool has_border_ = false;
    style border_style_{};
    std::string title_{};
    style title_style_{};
    align title_align_ = align::start;
    int pad_left_ = 0;
    int pad_top_ = 0;
    int pad_right_ = 0;
    int pad_bottom_ = 0;
    color background_{};
};

}  // namespace widgets
}  // namespace teacup
