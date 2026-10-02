#pragma once

#include <functional>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/glyph.hpp"

namespace teacup
{
namespace widgets
{

std::vector<GlyphString> wrap_text(const GlyphString& text, int width);

class Text
{
public:
    using RendererFn = std::function<void(Canvas)>;

    Text() = default;
    explicit Text(GlyphString content) : content_{ std::move(content) } { }

    Text& content(GlyphString value)
    {
        content_ = std::move(value);
        return *this;
    }

    Text& style(const Style& s)
    {
        style_ = s;
        return *this;
    }

    Text& align(Align a)
    {
        align_ = a;
        return *this;
    }

    Text& vertical_align(Align a)
    {
        valign_ = a;
        return *this;
    }

    Text& wrap(bool enabled)
    {
        wrap_ = enabled;
        return *this;
    }

    Text& renderer(RendererFn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    [[nodiscard]] int height_for(int width) const { return static_cast<int>(lines(width).size()); }

    void render(Canvas target) const
    {
        if (renderer_)
        {
            renderer_(target);
            return;
        }
        if (!target.valid())
        {
            return;
        }
        const int width = target.size().width;
        const int height = target.size().height;

        const std::vector<GlyphString> rows = lines(width);
        const int offset = align_offset(valign_, height, static_cast<int>(rows.size()));
        for (std::size_t i = 0; i < rows.size(); ++i)
        {
            const int y = offset + static_cast<int>(i);
            if (y >= 0 && y < height)
            {
                target.text_aligned(y, rows[i], align_, style_);
            }
        }
    }

private:
    [[nodiscard]] std::vector<GlyphString> lines(int width) const
    {
        if (wrap_)
        {
            return wrap_text(content_, width);
        }
        return wrap_text(content_, 0);
    }

    GlyphString content_{};
    Style style_{};
    Align align_ = Align::start;
    Align valign_ = Align::start;
    bool wrap_ = true;
    RendererFn renderer_;
};

}  // namespace widgets
}  // namespace teacup
