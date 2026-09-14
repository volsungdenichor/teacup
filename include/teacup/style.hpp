#pragma once

#include <cstdint>
#include <string>
#include <utility>

#include "teacup/color.hpp"

namespace teacup
{

enum class attr : std::uint16_t
{
    none = 0,
    bold = 1u << 0,
    dim = 1u << 1,
    italic = 1u << 2,
    underline = 1u << 3,
    blink = 1u << 4,
    reverse = 1u << 5,
    strikethrough = 1u << 6
};

constexpr attr operator|(attr l, attr r)
{
    return static_cast<attr>(static_cast<std::uint16_t>(l) | static_cast<std::uint16_t>(r));
}
constexpr attr operator&(attr l, attr r)
{
    return static_cast<attr>(static_cast<std::uint16_t>(l) & static_cast<std::uint16_t>(r));
}
constexpr bool has(attr set, attr flag)
{
    return (set & flag) == flag && flag != attr::none;
}

/// Cell-level appearance: foreground, background and text attributes.
class style
{
public:
    constexpr style() = default;

    style& fg(const color& c) &
    {
        fg_ = c;
        return *this;
    }
    style&& fg(const color& c) &&
    {
        fg_ = c;
        return std::move(*this);
    }
    style& bg(const color& c) &
    {
        bg_ = c;
        return *this;
    }
    style&& bg(const color& c) &&
    {
        bg_ = c;
        return std::move(*this);
    }

    style& add(attr a) &
    {
        attrs_ = attrs_ | a;
        return *this;
    }
    style&& add(attr a) &&
    {
        attrs_ = attrs_ | a;
        return std::move(*this);
    }

    style& bold() &
    {
        return add(attr::bold);
    }
    style&& bold() &&
    {
        return std::move(add(attr::bold));
    }
    style& dim() &
    {
        return add(attr::dim);
    }
    style&& dim() &&
    {
        return std::move(add(attr::dim));
    }
    style& italic() &
    {
        return add(attr::italic);
    }
    style&& italic() &&
    {
        return std::move(add(attr::italic));
    }
    style& underline() &
    {
        return add(attr::underline);
    }
    style&& underline() &&
    {
        return std::move(add(attr::underline));
    }
    style& reverse() &
    {
        return add(attr::reverse);
    }
    style&& reverse() &&
    {
        return std::move(add(attr::reverse));
    }

    [[nodiscard]] const color& foreground() const
    {
        return fg_;
    }
    [[nodiscard]] const color& background() const
    {
        return bg_;
    }
    [[nodiscard]] attr attributes() const
    {
        return attrs_;
    }

    /// Overlays `other` on top of `*this`: set colors and attributes win.
    [[nodiscard]] style merged(const style& other) const
    {
        style result = *this;
        if (other.fg_.is_set())
        {
            result.fg_ = other.fg_;
        }
        if (other.bg_.is_set())
        {
            result.bg_ = other.bg_;
        }
        result.attrs_ = result.attrs_ | other.attrs_;
        return result;
    }

    friend style operator|(const style& l, const style& r)
    {
        return l.merged(r);
    }

    friend bool operator==(const style& l, const style& r)
    {
        return l.fg_ == r.fg_ && l.bg_ == r.bg_ && l.attrs_ == r.attrs_;
    }
    friend bool operator!=(const style& l, const style& r)
    {
        return !(l == r);
    }

private:
    color fg_{};
    color bg_{};
    attr attrs_ = attr::none;
};

/// Full SGR sequence (including ESC[ ... m) that resets and applies `s`.
std::string sgr_sequence(const style& s, color_profile profile);

}  // namespace teacup
