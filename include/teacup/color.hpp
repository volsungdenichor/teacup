#pragma once

#include <cstdint>
#include <string>

namespace teacup
{

/// Terminal color profile, used to degrade truecolor gracefully.
enum class color_profile
{
    no_color,
    ansi16,
    ansi256,
    truecolor
};

/// A color that is either unset ("default"), a palette index or an RGB triple.
class color
{
public:
    enum class kind : std::uint8_t
    {
        unset,
        indexed,
        rgb
    };

    constexpr color() = default;

    static constexpr color unset()
    {
        return color{};
    }

    static constexpr color indexed(std::uint8_t index)
    {
        return color{ kind::indexed, index, 0, 0 };
    }

    static constexpr color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b)
    {
        return color{ kind::rgb, r, g, b };
    }

    /// Parses "#rgb", "#rrggbb" (with or without '#'). Returns unset on failure.
    static color hex(const std::string& text);

    /// Linear interpolation in sRGB space, `t` clamped to [0, 1].
    static color mix(const color& from, const color& to, double t);

    [[nodiscard]] constexpr kind type() const
    {
        return kind_;
    }
    [[nodiscard]] constexpr bool is_set() const
    {
        return kind_ != kind::unset;
    }
    [[nodiscard]] constexpr std::uint8_t index() const
    {
        return r_;
    }
    [[nodiscard]] constexpr std::uint8_t red() const
    {
        return r_;
    }
    [[nodiscard]] constexpr std::uint8_t green() const
    {
        return g_;
    }
    [[nodiscard]] constexpr std::uint8_t blue() const
    {
        return b_;
    }

    /// Converts the color so that it can be displayed under `profile`.
    [[nodiscard]] color adapt(color_profile profile) const;

    /// Emits the SGR parameters (without ESC[ and final 'm').
    [[nodiscard]] std::string sgr(bool foreground, color_profile profile) const;

    friend constexpr bool operator==(const color& l, const color& r)
    {
        return l.kind_ == r.kind_ && l.r_ == r.r_ && l.g_ == r.g_ && l.b_ == r.b_;
    }
    friend constexpr bool operator!=(const color& l, const color& r)
    {
        return !(l == r);
    }

private:
    constexpr color(kind k, std::uint8_t r, std::uint8_t g, std::uint8_t b)
        : kind_{ k }
        , r_{ r }
        , g_{ g }
        , b_{ b }
    {
    }

    kind kind_ = kind::unset;
    std::uint8_t r_ = 0;
    std::uint8_t g_ = 0;
    std::uint8_t b_ = 0;
};

/// Detects the profile from the environment ($NO_COLOR, $COLORTERM, $TERM).
color_profile detect_color_profile();

namespace palette
{

inline color subtle()
{
    return color::hex("#6c7086");
}
inline color text()
{
    return color::hex("#cdd6f4");
}
inline color accent()
{
    return color::hex("#7d56f4");
}
inline color success()
{
    return color::hex("#a6e3a1");
}
inline color warning()
{
    return color::hex("#f9e2af");
}
inline color danger()
{
    return color::hex("#f38ba8");
}
inline color surface()
{
    return color::hex("#1e1e2e");
}

}  // namespace palette

}  // namespace teacup
