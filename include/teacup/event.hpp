#pragma once

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <vector>

#include "teacup/geometry.hpp"

namespace teacup
{

enum class Key
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

enum class Modifier : std::uint8_t
{
    none = 0,
    shift = 1u << 0,
    alt = 1u << 1,
    ctrl = 1u << 2
};

constexpr Modifier operator|(Modifier lhs, Modifier rhs)
{
    return static_cast<Modifier>(static_cast<std::uint8_t>(lhs) | static_cast<std::uint8_t>(rhs));
}
constexpr bool has(Modifier set, Modifier flag)
{
    return (static_cast<std::uint8_t>(set) & static_cast<std::uint8_t>(flag)) != 0;
}

struct KeyEvent
{
    Key code = Key::none;
    std::string text{};
    Modifier modifiers = Modifier::none;

    [[nodiscard]] std::string name() const;

    [[nodiscard]] bool matches(const std::string& spec) const { return name() == spec; }
    [[nodiscard]] bool matches_any(const std::vector<std::string>& specs) const
    {
        return std::any_of(specs.begin(), specs.end(), [this](const std::string& spec) { return matches(spec); });
    }
};

enum class MouseButton
{
    none,
    left,
    middle,
    right,
    wheel_up,
    wheel_down
};

enum class MouseAction
{
    press,
    release,
    motion
};

struct MouseEvent
{
    Point position{};
    MouseButton button = MouseButton::none;
    MouseAction action = MouseAction::press;
    Modifier modifiers = Modifier::none;
};

struct ResizeEvent
{
    Size size{};
};

struct FocusEvent
{
    bool focused = true;
};

struct PasteEvent
{
    std::string text{};
};

struct StartEvent
{
};

}  // namespace teacup
