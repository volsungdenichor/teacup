#pragma once

#include <algorithm>
#include <string>
#include <utility>

#include "teacup/canvas.hpp"

namespace teacup
{
namespace widgets
{

/// A single-row progress bar with an optional color gradient.
class progress
{
public:
    explicit progress(double ratio = 0.0)
        : ratio_{ std::clamp(ratio, 0.0, 1.0) }
    {
    }

    progress& ratio(double value) &
    {
        ratio_ = std::clamp(value, 0.0, 1.0);
        return *this;
    }
    progress&& ratio(double value) &&
    {
        return std::move(this->ratio(value));
    }

    progress& gradient(const color& from, const color& to) &
    {
        from_ = from;
        to_ = to;
        return *this;
    }
    progress&& gradient(const color& from, const color& to) &&
    {
        return std::move(this->gradient(from, to));
    }

    progress& track_style(const style& s) &
    {
        track_style_ = s;
        return *this;
    }
    progress&& track_style(const style& s) &&
    {
        return std::move(this->track_style(s));
    }

    progress& glyphs(std::string filled, std::string empty) &
    {
        filled_ = std::move(filled);
        empty_ = std::move(empty);
        return *this;
    }
    progress&& glyphs(std::string filled, std::string empty) &&
    {
        return std::move(this->glyphs(std::move(filled), std::move(empty)));
    }

    void render(canvas target, int y = 0) const
    {
        const int width = target.width();
        if (width <= 0)
        {
            return;
        }
        const int filled = static_cast<int>(ratio_ * width + 0.5);
        for (int x = 0; x < width; ++x)
        {
            if (x < filled)
            {
                const double t = width > 1 ? static_cast<double>(x) / (width - 1) : 0.0;
                const style s = style{}.fg(color::mix(from_, to_, t));
                target.put(x, y, cell{ filled_, s, 1 });
            }
            else
            {
                target.put(x, y, cell{ empty_, track_style_, 1 });
            }
        }
    }

private:
    double ratio_ = 0.0;
    color from_ = color::hex("#7d56f4");
    color to_ = color::hex("#f38ba8");
    style track_style_ = style{}.fg(color::hex("#45475a"));
    std::string filled_ = "█";
    std::string empty_ = "░";
};

}  // namespace widgets
}  // namespace teacup
