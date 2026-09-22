#pragma once

#include <memory>
#include <string>
#include <utility>

#include "teacup/color.hpp"
#include "teacup/geometry.hpp"

namespace teacup
{
namespace detail
{

class Terminal
{
public:
    virtual ~Terminal() = default;

    virtual bool is_tty() const = 0;
    virtual Size size() const = 0;

    virtual void enter_raw_mode() = 0;
    virtual void leave_raw_mode() = 0;

    virtual void write(const std::string& data) = 0;
    virtual void flush() = 0;

    virtual std::pair<std::string, bool> read(int timeout_ms) = 0;

    virtual void set_alt_screen(bool enabled) = 0;
    virtual void set_cursor_visible(bool visible) = 0;
    virtual void set_mouse_enabled(bool enabled) = 0;
    virtual void set_bracketed_paste(bool enabled) = 0;
};

std::unique_ptr<Terminal> make_terminal();

}  // namespace detail
}  // namespace teacup
