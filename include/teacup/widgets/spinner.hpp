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

struct spinner
{
    struct state_type
    {
        std::size_t frame = 0;
    };

    /// View configuration for an application-owned spinner_state.
    class view_type
    {
    public:
        using renderer_fn = std::function<void(const state_type&, canvas)>;

        view_type& frames(std::vector<std::string> value)
        {
            frames_ = std::move(value);
            return *this;
        }
        view_type& style(const teacup::style& value)
        {
            style_ = value;
            return *this;
        }

        view_type& renderer(renderer_fn value)
        {
            renderer_ = std::move(value);
            return *this;
        }
        void advance(state_type& state) const
        {
            if (!frames_.empty())
            {
                state.frame = (state.frame + 1) % frames_.size();
            }
        }
        void render(const state_type& state, canvas target) const
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
            target.text(0, 0, frames_[state.frame % frames_.size()], style_);
        }

    private:
        std::vector<std::string> frames_{ "|", "/", "-", "\\" };
        teacup::style style_;
        renderer_fn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup