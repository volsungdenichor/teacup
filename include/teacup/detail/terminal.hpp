#pragma once

#include <memory>
#include <string>

#include "teacup/color.hpp"
#include "teacup/geometry.hpp"

namespace teacup
{
namespace detail
{

/// Abstraction over the host terminal; the only platform dependent piece.
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

    /// Blocks up to `timeout_ms` for input; returns bytes read (may be empty).
    /// `resized` is set when a resize was observed while waiting.
    virtual std::string read(int timeout_ms, bool& resized) = 0;

    virtual void set_alt_screen(bool enabled) = 0;
    virtual void set_cursor_visible(bool visible) = 0;
    virtual void set_mouse_enabled(bool enabled) = 0;
    virtual void set_bracketed_paste(bool enabled) = 0;
};

/// Creates the terminal implementation for the current platform.
std::unique_ptr<Terminal> make_terminal();

}  // namespace detail
}  // namespace teacup
