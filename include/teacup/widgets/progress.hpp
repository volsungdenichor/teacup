#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

#include "teacup/canvas.hpp"

namespace teacup
{
namespace widgets
{

struct Progress
{
    struct State
    {
        double ratio = 0.0;

        void clamp() { ratio = std::clamp(ratio, 0.0, 1.0); }
    };

    class View
    {
    public:
        using RendererFn = std::function<void(const State&, Canvas)>;

        View& gradient(const Color& from, const Color& to)
        {
            style_.from = from;
            style_.to = to;
            return *this;
        }

        View& track_style(const Style& s)
        {
            style_.track_style = s;
            return *this;
        }

        View& glyphs(Glyph filled, Glyph empty)
        {
            style_.filled = std::move(filled);
            style_.empty = std::move(empty);
            return *this;
        }

        View& renderer(RendererFn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        void render(const State& state, Canvas target, int y = 0) const
        {
            if (renderer_)
            {
                renderer_(state, target);
                return;
            }
            const int width = target.size().width;
            if (width <= 0)
            {
                return;
            }
            const double ratio = std::clamp(state.ratio, 0.0, 1.0);
            const int filled = static_cast<int>(ratio * width + 0.5);
            for (int x = 0; x < width; ++x)
            {
                if (x < filled)
                {
                    const double t = width > 1 ? static_cast<double>(x) / (width - 1) : 0.0;
                    const Style s = Style{}.fg(Color::mix(style_.from, style_.to, t));
                    target.put(Point{ x, y }, Cell{ style_.filled, s });
                }
                else
                {
                    target.put(Point{ x, y }, Cell{ style_.empty, style_.track_style });
                }
            }
        }

    private:
        struct WidgetStyle
        {
            Color from = Color::hex("#7d56f4");
            Color to = Color::hex("#f38ba8");
            Style track_style = Style{}.fg(Color::hex("#45475a"));
            Glyph filled = "█";
            Glyph empty = "░";
        };

        WidgetStyle style_;
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup
