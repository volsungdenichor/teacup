#pragma once

#include <chrono>

#include "teacup/canvas.hpp"
#include "teacup/color.hpp"
#include "teacup/command.hpp"
#include "teacup/event.hpp"
#include "teacup/message.hpp"

namespace teacup
{

struct Options
{
    bool alt_screen = true;
    bool hide_cursor = true;
    bool mouse = false;
    bool bracketed_paste = true;
    std::chrono::milliseconds frame_interval{ 16 };
    ColorProfile profile = detect_color_profile();
};

namespace detail
{

class AppBase
{
public:
    virtual ~AppBase() = default;
    virtual Cmd init() = 0;
    virtual Cmd update(const Message& msg) = 0;
    virtual void view(Canvas target) const = 0;
};

int run_app(AppBase& app, const Options& opts);

}  // namespace detail

template <typename App>
int run(App&& app, const Options& opts = {})
{
    detail::AppBase& base = app;
    return detail::run_app(base, opts);
}

}  // namespace teacup
