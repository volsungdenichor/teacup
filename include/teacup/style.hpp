#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <utility>

#include "teacup/color.hpp"

namespace teacup
{

enum class Attr : std::uint16_t
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

constexpr Attr operator|(Attr lhs, Attr rhs)
{
    return static_cast<Attr>(static_cast<std::uint16_t>(lhs) | static_cast<std::uint16_t>(rhs));
}
constexpr Attr operator&(Attr lhs, Attr rhs)
{
    return static_cast<Attr>(static_cast<std::uint16_t>(lhs) & static_cast<std::uint16_t>(rhs));
}
constexpr bool has(Attr set, Attr flag)
{
    return (set & flag) == flag && flag != Attr::none;
}

class Style
{
public:
    constexpr Style() = default;

    Style& fg(const Color& c) &
    {
        fg_ = c;
        return *this;
    }
    Style&& fg(const Color& c) &&
    {
        fg_ = c;
        return std::move(*this);
    }
    Style& bg(const Color& c) &
    {
        bg_ = c;
        return *this;
    }
    Style&& bg(const Color& c) &&
    {
        bg_ = c;
        return std::move(*this);
    }

    Style& add(Attr a) &
    {
        attrs_ = attrs_ | a;
        return *this;
    }
    Style&& add(Attr a) &&
    {
        attrs_ = attrs_ | a;
        return std::move(*this);
    }

    Style& bold() & { return add(Attr::bold); }
    Style&& bold() && { return std::move(add(Attr::bold)); }
    Style& dim() & { return add(Attr::dim); }
    Style&& dim() && { return std::move(add(Attr::dim)); }
    Style& italic() & { return add(Attr::italic); }
    Style&& italic() && { return std::move(add(Attr::italic)); }
    Style& underline() & { return add(Attr::underline); }
    Style&& underline() && { return std::move(add(Attr::underline)); }
    Style& reverse() & { return add(Attr::reverse); }
    Style&& reverse() && { return std::move(add(Attr::reverse)); }

    [[nodiscard]] const Color& foreground() const { return fg_; }
    [[nodiscard]] const Color& background() const { return bg_; }
    [[nodiscard]] Attr attributes() const { return attrs_; }

    Style& merge(const Style& other)
    {
        if (other.fg_.is_set())
        {
            fg_ = other.fg_;
        }
        if (other.bg_.is_set())
        {
            bg_ = other.bg_;
        }
        attrs_ = attrs_ | other.attrs_;
        return *this;
    }

    [[nodiscard]] Style merged(const Style& other) const
    {
        Style result = *this;
        result.merge(other);
        return result;
    }

    friend Style& operator|=(Style& lhs, const Style& rhs) { return lhs.merge(rhs); }
    friend Style operator|(const Style& lhs, const Style& rhs) { return lhs.merged(rhs); }

    friend bool operator==(const Style& lhs, const Style& rhs)
    {
        return std::tie(lhs.fg_, lhs.bg_, lhs.attrs_) == std::tie(rhs.fg_, rhs.bg_, rhs.attrs_);
    }
    friend bool operator!=(const Style& lhs, const Style& rhs) { return !(lhs == rhs); }

private:
    Color fg_{};
    Color bg_{};
    Attr attrs_ = Attr::none;
};

/// Full SGR sequence (including ESC[ ... m) that resets and applies `s`.
std::string sgr_sequence(const Style& s, ColorProfile profile);

}  // namespace teacup
