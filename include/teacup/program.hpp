#pragma once

#include <chrono>

#include "teacup/canvas.hpp"
#include "teacup/color.hpp"
#include "teacup/command.hpp"
#include "teacup/event.hpp"
#include "teacup/message.hpp"

namespace teacup
{

/// Runtime configuration of a program.
struct options
{
    /// Render on the alternate screen buffer (restores the shell on exit).
    bool alt_screen = true;
    bool hide_cursor = true;
    bool mouse = false;
    bool bracketed_paste = true;
    /// Upper bound on the repaint rate.
    std::chrono::milliseconds frame_interval{ 16 };
    /// Color depth; detected from the environment by default.
    color_profile profile = detect_color_profile();
};

namespace detail
{

/// Type-erased application seen by the runtime.
class app_base
{
public:
    virtual ~app_base() = default;
    virtual cmd init() = 0;
    virtual cmd update(const message& msg) = 0;
    virtual void view(canvas target) const = 0;
};

int run_app(app_base& app, const options& opts);

}  // namespace detail

/// Runs `app` until it issues `quit()`. Returns 0 on success.
template <typename App>
int run(App&& app, const options& opts = {})
{
    detail::app_base& base = app;
    return detail::run_app(base, opts);
}

}  // namespace teacup
