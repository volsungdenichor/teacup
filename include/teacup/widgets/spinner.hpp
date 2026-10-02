#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct Spinner
{
    struct State
    {
        std::size_t frame = 0;
    };

    class View
    {
    public:
        using RendererFn = std::function<void(const State&, Canvas)>;

        View& frames(std::vector<GlyphString> value)
        {
            frames_ = std::move(value);
            return *this;
        }

        View& style(const Style& value)
        {
            style_ = value;
            return *this;
        }

        View& renderer(RendererFn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        void advance(State& state) const
        {
            if (!frames_.empty())
            {
                state.frame = (state.frame + 1) % frames_.size();
            }
        }

        void render(const State& state, Canvas target) const
        {
            if (renderer_)
            {
                renderer_(state, target);
                return;
            }
            if (!target.valid() || frames_.empty())
            {
                return;
            }
            target.text(Point{ 0, 0 }, frames_[state.frame % frames_.size()], style_);
        }

    private:
        std::vector<GlyphString> frames_{ "|", "/", "-", "\\" };
        Style style_;
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup