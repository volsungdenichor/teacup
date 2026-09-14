#pragma once

#include <string>

#include "teacup/color.hpp"
#include "teacup/detail/terminal.hpp"
#include "teacup/surface.hpp"

namespace teacup
{
namespace detail
{

/// Double-buffered renderer: diffs the new frame against the previous one and
/// emits the minimal amount of cursor moves and SGR changes.
class renderer
{
public:
    renderer(terminal& out, color_profile profile);

    void set_profile(color_profile profile)
    {
        profile_ = profile;
    }

    /// Forces a full repaint on the next `draw` (after resize or alt screen).
    void invalidate();

    void draw(const surface& frame);

    /// Builds the escape sequence for a frame without writing it (test hook).
    [[nodiscard]] std::string build(const surface& frame);

private:
    terminal& out_;
    color_profile profile_;
    surface previous_{};
    bool full_repaint_ = true;
};

}  // namespace detail
}  // namespace teacup
