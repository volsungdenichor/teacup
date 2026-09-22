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

class Cmd
{
public:
    using Function = std::function<Message()>;

    Cmd() = default;

    Cmd(Function fn)  // NOLINT(google-explicit-constructor): ergonomics
        : items_{ std::move(fn) }
    {
    }

    static Cmd none() { return Cmd{}; }

    [[nodiscard]] bool empty() const { return items_.empty(); }

    [[nodiscard]] const std::vector<Function>& items() const { return items_; }

    /// Concatenates two commands; both run concurrently.
    Cmd& operator+=(const Cmd& other)
    {
        items_.insert(items_.end(), other.items_.begin(), other.items_.end());
        return *this;
    }

    friend Cmd operator+(Cmd l, const Cmd& r)
    {
        l += r;
        return l;
    }

private:
    std::vector<Function> items_{};
};

template <class... Cmds>
Cmd batch(Cmds&&... cmds)
{
    Cmd result;
    const int expander[] = { 0, (result += cmds, 0)... };
    static_cast<void>(expander);
    return result;
}

Cmd sequence(std::vector<Cmd> cmds);
struct QuitMessage
{
};
struct SetAltScreenMessage
{
    bool enabled = true;
};
struct SetCursorVisibleMessage
{
    bool visible = false;
};
struct SetMouseEnabledMessage
{
    bool enabled = true;
};

inline Cmd quit()
{
    return Cmd{ [] { return Message{ QuitMessage{} }; } };
}

inline Cmd enter_alt_screen()
{
    return Cmd{ [] { return Message{ SetAltScreenMessage{ true } }; } };
}
inline Cmd exit_alt_screen()
{
    return Cmd{ [] { return Message{ SetAltScreenMessage{ false } }; } };
}
inline Cmd show_cursor()
{
    return Cmd{ [] { return Message{ SetCursorVisibleMessage{ true } }; } };
}
inline Cmd hide_cursor()
{
    return Cmd{ [] { return Message{ SetCursorVisibleMessage{ false } }; } };
}
inline Cmd enable_mouse()
{
    return Cmd{ [] { return Message{ SetMouseEnabledMessage{ true } }; } };
}
inline Cmd disable_mouse()
{
    return Cmd{ [] { return Message{ SetMouseEnabledMessage{ false } }; } };
}

/// Emits `fn()` once after `delay`.
template <class Rep, class Period, class Fn>
Cmd tick(std::chrono::duration<Rep, Period> delay, Fn fn)
{
    return Cmd{ [delay, fn = std::move(fn)]() -> Message
                {
                    std::this_thread::sleep_for(delay);
                    return Message{ fn() };
                } };
}

template <class Fn>
Cmd perform(Fn fn)
{
    return Cmd{ [fn = std::move(fn)]() -> Message { return Message{ fn() }; } };
}

}  // namespace teacup
