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

struct spinner_state
{
    std::size_t frame = 0;
};

/// View configuration for an application-owned spinner_state.
class spinner_view
{
public:
    using renderer_fn = std::function<void(const spinner_state&, canvas)>;

    spinner_view& frames(std::vector<std::string> value)
    {
        frames_ = std::move(value);
        return *this;
    }
    spinner_view& style(const teacup::style& value)
    {
        style_ = value;
        return *this;
    }

    spinner_view& renderer(renderer_fn value)
    {
        renderer_ = std::move(value);
        return *this;
    }
    void advance(spinner_state& state) const
    {
        if (!frames_.empty())
        {
            state.frame = (state.frame + 1) % frames_.size();
        }
    }
    void render(const spinner_state& state, canvas target) const
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

}  // namespace widgets
}  // namespace teacup