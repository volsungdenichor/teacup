#pragma once

#include <functional>
#include <string>
#include <utility>

#include "teacup/border.hpp"
#include "teacup/canvas.hpp"
#include "teacup/glyph.hpp"

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
        style_.border = b;
        style_.has_border = true;
        return *this;
    }

    Block& border_style(const Style& s)
    {
        style_.border_style = s;
        return *this;
    }

    Block& title(GlyphString text)
    {
        title_ = std::move(text);
        return *this;
    }

    Block& title_style(const Style& s)
    {
        style_.title_style = s;
        return *this;
    }

    Block& title_align(Align a)
    {
        style_.title_align = a;
        return *this;
    }

    Block& padding(int all) { return padding(all, all, all, all); }

    Block& padding(int left, int top, int right, int bottom)
    {
        style_.pad_left = left;
        style_.pad_top = top;
        style_.pad_right = right;
        style_.pad_bottom = bottom;
        return *this;
    }

    Block& background(const Color& c)
    {
        style_.background = c;
        return *this;
    }

    Block& renderer(RendererFn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    [[nodiscard]] Rect inner(const Rect& area) const
    {
        const int edge = style_.has_border ? 1 : 0;
        return area.inset(edge + style_.pad_left, edge + style_.pad_top, edge + style_.pad_right, edge + style_.pad_bottom);
    }

    Canvas draw(Canvas target) const
    {
        if (style_.background.is_set())
        {
            target.fill(" ", Style{}.bg(style_.background));
        }
        if (style_.has_border)
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
        const int w = target.size().width;
        const int h = target.size().height;
        if (w <= 0 || h <= 0)
        {
            return;
        }
        const Style& s = style_.border_style;

        if (h == 1)
        {
            target.horizontal_line(Point{ 0, 0 }, w, style_.border.top, s);
        }
        else
        {
            target.horizontal_line(Point{ 1, 0 }, w - 2, style_.border.top, s);
            target.horizontal_line(Point{ 1, h - 1 }, w - 2, style_.border.bottom, s);
            target.vertical_line(Point{ 0, 1 }, h - 2, style_.border.left, s);
            target.vertical_line(Point{ w - 1, 1 }, h - 2, style_.border.right, s);
            target.put(Point{ 0, 0 }, Cell{ style_.border.top_left, s });
            target.put(Point{ w - 1, 0 }, Cell{ style_.border.top_right, s });
            target.put(Point{ 0, h - 1 }, Cell{ style_.border.bottom_left, s });
            target.put(Point{ w - 1, h - 1 }, Cell{ style_.border.bottom_right, s });
        }

        if (!title_.empty() && w > 2)
        {
            const int available = w - 2;
            const GlyphString title = title_.truncate(available);
            const int offset = 1 + align_offset(style_.title_align, available, title.width());
            target.text(Point{ offset, 0 }, title, style_.title_style);
        }
    }

    struct WidgetStyle
    {
        Border border{};
        bool has_border = false;
        Style border_style{};
        Style title_style{};
        Align title_align = Align::start;
        int pad_left = 0;
        int pad_top = 0;
        int pad_right = 0;
        int pad_bottom = 0;
        Color background{};
    };

    GlyphString title_{};
    WidgetStyle style_{};
    RendererFn renderer_;
};

}  // namespace widgets
}  // namespace teacup
