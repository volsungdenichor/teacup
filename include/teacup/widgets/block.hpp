#pragma once

#include <functional>
#include <string>
#include <utility>

#include "teacup/border.hpp"
#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace widgets
{

class Block
{
public:
    using RendererFn = std::function<void(Canvas)>;

    Block& border(const Border& b)
    {
        border_ = b;
        has_border_ = true;
        return *this;
    }
    Block& border_style(const Style& s)
    {
        border_style_ = s;
        return *this;
    }
    Block& title(std::string text)
    {
        title_ = std::move(text);
        return *this;
    }
    Block& title_style(const Style& s)
    {
        title_style_ = s;
        return *this;
    }
    Block& title_align(Align a)
    {
        title_align_ = a;
        return *this;
    }
    Block& padding(int all) { return padding(all, all, all, all); }
    Block& padding(int left, int top, int right, int bottom)
    {
        pad_left_ = left;
        pad_top_ = top;
        pad_right_ = right;
        pad_bottom_ = bottom;
        return *this;
    }
    Block& background(const Color& c)
    {
        background_ = c;
        return *this;
    }

    Block& renderer(RendererFn value)
    {
        renderer_ = std::move(value);
        return *this;
    }
    [[nodiscard]] Rect inner(const Rect& area) const
    {
        const int edge = has_border_ ? 1 : 0;
        return area.inset(edge + pad_left_, edge + pad_top_, edge + pad_right_, edge + pad_bottom_);
    }

    Canvas draw(Canvas target) const
    {
        if (background_.is_set())
        {
            target.fill(" ", Style{}.bg(background_));
        }
        if (has_border_)
        {
            draw_border(target);
        }
        return target.sub(inner(target.bounds()));
    }

    template <typename Fn>
    void render(Canvas target, Fn&& content) const
    {
        Canvas inner_canvas = draw(target);
        if (inner_canvas.valid())
        {
            std::forward<Fn>(content)(inner_canvas);
        }
    }

    void render(Canvas target) const
    {
        Canvas inner_canvas = draw(target);
        if (inner_canvas.valid() && renderer_)
        {
            renderer_(inner_canvas);
        }
    }

private:
    void draw_border(Canvas& target) const
    {
        const int w = target.width();
        const int h = target.height();
        if (w <= 0 || h <= 0)
        {
            return;
        }
        const Style& s = border_style_;

        if (h == 1)
        {
            target.horizontal_line(Point{ 0, 0 }, w, border_.top, s);
        }
        else
        {
            target.horizontal_line(Point{ 1, 0 }, w - 2, border_.top, s);
            target.horizontal_line(Point{ 1, h - 1 }, w - 2, border_.bottom, s);
            target.vertical_line(Point{ 0, 1 }, h - 2, border_.left, s);
            target.vertical_line(Point{ w - 1, 1 }, h - 2, border_.right, s);
            target.put(Point{ 0, 0 }, Cell{ border_.top_left, s, 1 });
            target.put(Point{ w - 1, 0 }, Cell{ border_.top_right, s, 1 });
            target.put(Point{ 0, h - 1 }, Cell{ border_.bottom_left, s, 1 });
            target.put(Point{ w - 1, h - 1 }, Cell{ border_.bottom_right, s, 1 });
        }

        if (!title_.empty() && w > 2)
        {
            const int available = w - 2;
            const int extent = (std::min)(utf8::width(title_), available);
            const int offset = 1 + align_offset(title_align_, available, extent);
            target.text(Point{ offset, 0 }, title_, title_style_);
        }
    }

    Border border_{};
    bool has_border_ = false;
    Style border_style_{};
    std::string title_{};
    Style title_style_{};
    Align title_align_ = Align::start;
    int pad_left_ = 0;
    int pad_top_ = 0;
    int pad_right_ = 0;
    int pad_bottom_ = 0;
    Color background_{};
    RendererFn renderer_;
};

}  // namespace widgets
}  // namespace teacup
