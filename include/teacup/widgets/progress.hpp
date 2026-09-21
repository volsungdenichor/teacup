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

/// Application-owned progress state.

struct progress
{
    struct state_type
    {
        double ratio = 0.0;

        void clamp() { ratio = std::clamp(ratio, 0.0, 1.0); }
    };

    /// Rendering configuration and view for an application-owned progress_state.
    class view_type
    {
    public:
        using renderer_fn = std::function<void(const state_type&, canvas)>;

        view_type& gradient(const color& from, const color& to)
        {
            from_ = from;
            to_ = to;
            return *this;
        }
        view_type& track_style(const style& s)
        {
            track_style_ = s;
            return *this;
        }
        view_type& glyphs(std::string filled, std::string empty)
        {
            filled_ = std::move(filled);
            empty_ = std::move(empty);
            return *this;
        }
        view_type& renderer(renderer_fn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        void render(const state_type& state, canvas target, int y = 0) const
        {
            if (renderer_)
            {
                renderer_(state, target);
                return;
            }
            const int width = target.width();
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
                    const style s = style{}.fg(color::mix(from_, to_, t));
                    target.put(point{ x, y }, cell{ filled_, s, 1 });
                }
                else
                {
                    target.put(point{ x, y }, cell{ empty_, track_style_, 1 });
                }
            }
        }

    private:
        color from_ = color::hex("#7d56f4");
        color to_ = color::hex("#f38ba8");
        style track_style_ = style{}.fg(color::hex("#45475a"));
        std::string filled_ = "█";
        std::string empty_ = "░";
        renderer_fn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup
