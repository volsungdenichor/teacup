#pragma once

#include <cstdint>
#include <string>

namespace teacup
{

enum class ColorProfile
{
    no_color,
    ansi16,
    ansi256,
    truecolor
};

class Color
{
public:
    enum class Kind : std::uint8_t
    {
        unset,
        indexed,
        rgb
    };

    constexpr Color() = default;

    static constexpr Color unset() { return Color{}; }

    static constexpr Color indexed(std::uint8_t index) { return Color{ Kind::indexed, index, 0, 0 }; }

    static constexpr Color rgb(std::uint8_t r, std::uint8_t g, std::uint8_t b) { return Color{ Kind::rgb, r, g, b }; }

    /// Parses "#rgb", "#rrggbb" (with or without '#'). Returns unset on failure.
    static Color hex(const std::string& text);

    /// Linear interpolation in sRGB space, `t` clamped to [0, 1].
    static Color mix(const Color& from, const Color& to, double t);

    [[nodiscard]] constexpr Kind type() const { return kind_; }
    [[nodiscard]] constexpr bool is_set() const { return kind_ != Kind::unset; }
    [[nodiscard]] constexpr std::uint8_t index() const { return r_; }
    [[nodiscard]] constexpr std::uint8_t red() const { return r_; }
    [[nodiscard]] constexpr std::uint8_t green() const { return g_; }
    [[nodiscard]] constexpr std::uint8_t blue() const { return b_; }

    /// Converts the color so that it can be displayed under `profile`.
    [[nodiscard]] Color adapt(ColorProfile profile) const;

    /// Emits the SGR parameters (without ESC[ and final 'm').
    [[nodiscard]] std::string sgr(bool foreground, ColorProfile profile) const;

    friend constexpr bool operator==(const Color& l, const Color& r)
    {
        return l.kind_ == r.kind_ && l.r_ == r.r_ && l.g_ == r.g_ && l.b_ == r.b_;
    }
    friend constexpr bool operator!=(const Color& l, const Color& r) { return !(l == r); }

private:
    constexpr Color(Kind k, std::uint8_t r, std::uint8_t g, std::uint8_t b) : kind_{ k }, r_{ r }, g_{ g }, b_{ b } { }

    Kind kind_ = Kind::unset;
    std::uint8_t r_ = 0;
    std::uint8_t g_ = 0;
    std::uint8_t b_ = 0;
};

/// Detects the profile from the environment ($NO_COLOR, $COLORTERM, $TERM).
ColorProfile detect_color_profile();

namespace palette
{

inline Color subtle()
{
    return Color::hex("#6c7086");
}
inline Color text()
{
    return Color::hex("#cdd6f4");
}
inline Color accent()
{
    return Color::hex("#7d56f4");
}
inline Color success()
{
    return Color::hex("#a6e3a1");
}
inline Color warning()
{
    return Color::hex("#f9e2af");
}
inline Color danger()
{
    return Color::hex("#f38ba8");
}
inline Color surface()
{
    return Color::hex("#1e1e2e");
}

}  // namespace palette

}  // namespace teacup
