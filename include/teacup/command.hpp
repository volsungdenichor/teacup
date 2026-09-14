#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "teacup/message.hpp"

namespace teacup
{

/// An asynchronous side effect that eventually produces a message.
///
/// Commands are the only place where I/O, timers or blocking work belong; the
/// runtime executes them off the update thread and feeds their result back in.
class cmd
{
public:
    using function = std::function<message()>;

    cmd() = default;

    cmd(function fn)  // NOLINT(google-explicit-constructor): ergonomics
        : items_{ std::move(fn) }
    {
    }

    static cmd none()
    {
        return cmd{};
    }

    [[nodiscard]] bool empty() const
    {
        return items_.empty();
    }

    [[nodiscard]] const std::vector<function>& items() const
    {
        return items_;
    }

    /// Concatenates two commands; both run concurrently.
    cmd& operator+=(const cmd& other)
    {
        items_.insert(items_.end(), other.items_.begin(), other.items_.end());
        return *this;
    }

    friend cmd operator+(cmd l, const cmd& r)
    {
        l += r;
        return l;
    }

private:
    std::vector<function> items_{};
};

/// Runs every command concurrently.
template <typename... Cmds>
cmd batch(Cmds&&... cmds)
{
    cmd result;
    const int expander[] = { 0, (result += cmds, 0)... };
    static_cast<void>(expander);
    return result;
}

/// Runs commands one after another on a single worker; messages arrive in order.
cmd sequence(std::vector<cmd> cmds);

/// Internal message asking the runtime to stop; produced by `quit()`.
struct quit_message
{
};

/// Internal message toggling terminal features.
struct set_alt_screen_message
{
    bool enabled = true;
};
struct set_cursor_visible_message
{
    bool visible = false;
};
struct set_mouse_enabled_message
{
    bool enabled = true;
};

/// Stops the program and restores the terminal.
inline cmd quit()
{
    return cmd{ [] { return message{ quit_message{} }; } };
}

inline cmd enter_alt_screen()
{
    return cmd{ [] { return message{ set_alt_screen_message{ true } }; } };
}
inline cmd exit_alt_screen()
{
    return cmd{ [] { return message{ set_alt_screen_message{ false } }; } };
}
inline cmd show_cursor()
{
    return cmd{ [] { return message{ set_cursor_visible_message{ true } }; } };
}
inline cmd hide_cursor()
{
    return cmd{ [] { return message{ set_cursor_visible_message{ false } }; } };
}
inline cmd enable_mouse()
{
    return cmd{ [] { return message{ set_mouse_enabled_message{ true } }; } };
}
inline cmd disable_mouse()
{
    return cmd{ [] { return message{ set_mouse_enabled_message{ false } }; } };
}

/// Emits `fn()` once after `delay`.
template <typename Rep, typename Period, typename Fn>
cmd tick(std::chrono::duration<Rep, Period> delay, Fn fn)
{
    return cmd{ [delay, fn = std::move(fn)]() -> message {
        std::this_thread::sleep_for(delay);
        return message{ fn() };
    } };
}

/// Runs `fn` on a worker thread and delivers its result as a message.
template <typename Fn>
cmd perform(Fn fn)
{
    return cmd{ [fn = std::move(fn)]() -> message { return message{ fn() }; } };
}

}  // namespace teacup
