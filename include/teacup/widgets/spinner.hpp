#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

/// A single-cell frame spinner.
class spinner
{
public:
    spinner() = default;

    spinner& frames(std::vector<std::string> value) &
    {
        frames_ = std::move(value);
        normalize_frame();
        return *this;
    }
    spinner&& frames(std::vector<std::string> value) &&
    {
        return std::move(this->frames(std::move(value)));
    }

    spinner& frame(std::size_t value) &
    {
        frame_ = value;
        normalize_frame();
        return *this;
    }
    spinner&& frame(std::size_t value) &&
    {
        return std::move(this->frame(value));
    }

    spinner& style(const teacup::style& value) &
    {
        style_ = value;
        return *this;
    }
    spinner&& style(const teacup::style& value) &&
    {
        return std::move(this->style(value));
    }

    spinner& advance() &
    {
        if (!frames_.empty())
        {
            frame_ = (frame_ + 1) % frames_.size();
        }
        return *this;
    }
    spinner&& advance() &&
    {
        return std::move(this->advance());
    }

    [[nodiscard]] std::size_t frame() const
    {
        return frame_;
    }

    void render(canvas target) const
    {
        if (!target.valid() || frames_.empty())
        {
            return;
        }
        target.text(0, 0, frames_[frame_], style_);
    }

private:
    void normalize_frame()
    {
        if (frames_.empty())
        {
            frame_ = 0;
        }
        else
        {
            frame_ %= frames_.size();
        }
    }

    std::vector<std::string> frames_{ "|", "/", "-", "\\" };
    std::size_t frame_ = 0;
    teacup::style style_;
};

}  // namespace widgets
}  // namespace teacup