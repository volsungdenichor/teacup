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

    /// Forces a full repaint on the next `draw` (after resize or alt screen).
    void invalidate();

    void draw(const Surface& frame);

    /// Builds the escape sequence for a frame without writing it (test hook).
    [[nodiscard]] std::string build(const Surface& frame);

private:
    Terminal& out_;
    ColorProfile profile_;
    Surface previous_{};
    bool full_repaint_ = true;
};

}  // namespace detail
}  // namespace teacup
