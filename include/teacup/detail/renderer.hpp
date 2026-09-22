#pragma once

#include <string>

#include "teacup/color.hpp"
#include "teacup/detail/terminal.hpp"
#include "teacup/surface.hpp"

namespace teacup
{
namespace detail
{

class Renderer
{
public:
    Renderer(Terminal& out, ColorProfile profile);

    void set_profile(ColorProfile profile) { profile_ = profile; }

    void invalidate();

    void draw(const Surface& frame);

    [[nodiscard]] std::string build(const Surface& frame);

private:
    Terminal& out_;
    ColorProfile profile_;
    Surface previous_{};
    bool full_repaint_ = true;
};

}  // namespace detail
}  // namespace teacup
