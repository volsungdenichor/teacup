#pragma once

#include <cstdint>
#include <string>

#include "teacup/geometry.hpp"

namespace teacup
{

enum class key
{
    none,
    character,
    enter,
    tab,
    backspace,
    escape,
    space,
    up,
    down,
    left,
    right,
    home,
    end,
    page_up,
    page_down,
    insert,
    del,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12
};

enum class modifier : std::uint8_t
{
    none = 0,
    shift = 1u << 0,
    alt = 1u << 1,
    ctrl = 1u << 2
};

constexpr modifier operator|(modifier l, modifier r)
{
    return static_cast<modifier>(
        static_cast<std::uint8_t>(l) | static_cast<std::uint8_t>(r));
}
constexpr bool has(modifier set, modifier flag)
{
    return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(flag)) != 0;
}

struct key_event
{
    teacup::key code = key::none;
    /// UTF-8 text of the key when `code == key::character`.
    std::string text{};
    teacup::modifier modifiers = modifier::none;

    /// Canonical name, e.g. "ctrl+c", "shift+tab", "up", "a".
    [[nodiscard]] std::string name() const;

    /// Compares against a canonical name; case sensitive for characters.
    [[nodiscard]] bool matches(const std::string& spec) const
    {
        return name() == spec;
    }
};

enum class mouse_button
{
    none,
    left,
    middle,
    right,
    wheel_up,
    wheel_down
};

enum class mouse_action
{
    press,
    release,
    motion
};

struct mouse_event
{
    point position{};
    mouse_button button = mouse_button::none;
    mouse_action action = mouse_action::press;
    teacup::modifier modifiers = modifier::none;
};

struct resize_event
{
    teacup::size size{};
};

struct focus_event
{
    bool focused = true;
};

struct paste_event
{
    std::string text{};
};

/// Emitted once before the first render.
struct start_event
{
};

}  // namespace teacup
