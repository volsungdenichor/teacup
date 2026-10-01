#pragma once

#include <cstdint>
#include <string>
#include <array>

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

    static Color hex(const std::string& text);

    static Color mix(const Color& from, const Color& to, double t);

    [[nodiscard]] constexpr Kind type() const { return kind_; }
    [[nodiscard]] constexpr bool is_set() const { return kind_ != Kind::unset; }
    [[nodiscard]] constexpr std::uint8_t index() const { return values_[0]; }
    [[nodiscard]] constexpr std::uint8_t red() const { return values_[0]; }
    [[nodiscard]] constexpr std::uint8_t green() const { return values_[1]; }
    [[nodiscard]] constexpr std::uint8_t blue() const { return values_[2]; }

    [[nodiscard]] Color adapt(ColorProfile profile) const;

    /// Emits the SGR parameters (without ESC[ and final 'm').
    [[nodiscard]] std::string sgr(bool foreground, ColorProfile profile) const;

    friend constexpr bool operator==(const Color& l, const Color& r)
    {
        return l.kind_ == r.kind_ && l.values_ == r.values_;
    }
    friend constexpr bool operator!=(const Color& l, const Color& r) { return !(l == r); }

private:
    constexpr Color(Kind k, std::uint8_t r, std::uint8_t g, std::uint8_t b) : kind_{ k }, values_{ r, g, b } { }

    Kind kind_ = Kind::unset;
    std::array<std::uint8_t, 3> values_ = {};
};

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
