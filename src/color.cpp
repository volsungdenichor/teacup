#include "teacup/color.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace teacup
{
namespace
{

int hex_digit(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    return -1;
}

std::uint8_t to_u8(int v)
{
    return static_cast<std::uint8_t>(std::clamp(v, 0, 255));
}

std::uint8_t cube_component(std::uint8_t v)
{
    static const int levels[] = { 0, 95, 135, 175, 215, 255 };
    int best = 0;
    int best_dist = 1 << 30;
    for (int i = 0; i < 6; ++i)
    {
        const int dist = std::abs(levels[i] - static_cast<int>(v));
        if (dist < best_dist)
        {
            best_dist = dist;
            best = i;
        }
    }
    return static_cast<std::uint8_t>(best);
}

std::uint8_t rgb_to_ansi256(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    if (r == g && g == b)
    {
        if (r < 8)
        {
            return 16;
        }
        if (r > 248)
        {
            return 231;
        }
        return static_cast<std::uint8_t>(232 + ((static_cast<int>(r) - 8) * 24) / 247);
    }
    return static_cast<std::uint8_t>(16 + 36 * cube_component(r) + 6 * cube_component(g) + cube_component(b));
}

std::uint8_t rgb_to_ansi16(std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    const int luma = (299 * r + 587 * g + 114 * b) / 1000;
    const int threshold = 128;
    int code = (r > threshold ? 1 : 0) | (g > threshold ? 2 : 0) | (b > threshold ? 4 : 0);
    if (luma > 192)
    {
        code |= 8;
    }
    return static_cast<std::uint8_t>(code);
}

std::uint8_t ansi256_to_ansi16(std::uint8_t index)
{
    if (index < 16)
    {
        return index;
    }
    if (index >= 232)
    {
        const int level = 8 + (index - 232) * 10;
        return rgb_to_ansi16(to_u8(level), to_u8(level), to_u8(level));
    }
    static const int levels[] = { 0, 95, 135, 175, 215, 255 };
    const int n = index - 16;
    return rgb_to_ansi16(to_u8(levels[n / 36]), to_u8(levels[(n / 6) % 6]), to_u8(levels[n % 6]));
}

bool env_contains(const char* name, const char* needle)
{
    const char* value = std::getenv(name);
    return value != nullptr && std::strstr(value, needle) != nullptr;
}

}  // namespace

Color Color::hex(const std::string& text)
{
    std::string body = text;
    if (!body.empty() && body.front() == '#')
    {
        body.erase(body.begin());
    }
    const auto digits = [&](std::size_t i) { return hex_digit(body[i]); };
    if (body.size() == 3)
    {
        const int r = digits(0), g = digits(1), b = digits(2);
        if (r < 0 || g < 0 || b < 0)
        {
            return {};
        }
        return rgb(to_u8(r * 17), to_u8(g * 17), to_u8(b * 17));
    }
    if (body.size() == 6)
    {
        int c[6];
        for (std::size_t i = 0; i < 6; ++i)
        {
            c[i] = digits(i);
            if (c[i] < 0)
            {
                return {};
            }
        }
        return rgb(to_u8(c[0] * 16 + c[1]), to_u8(c[2] * 16 + c[3]), to_u8(c[4] * 16 + c[5]));
    }
    return {};
}

Color Color::mix(const Color& from, const Color& to, double t)
{
    if (from.type() != Kind::rgb || to.type() != Kind::rgb)
    {
        return t < 0.5 ? from : to;
    }
    const double f = std::clamp(t, 0.0, 1.0);
    const auto lerp = [&](std::uint8_t a, std::uint8_t b) { return to_u8(static_cast<int>(std::lround(a + (b - a) * f))); };
    return rgb(lerp(from.red(), to.red()), lerp(from.green(), to.green()), lerp(from.blue(), to.blue()));
}

Color Color::adapt(ColorProfile profile) const
{
    switch (profile)
    {
        case ColorProfile::no_color: return {};
        case ColorProfile::truecolor: return *this;
        case ColorProfile::ansi256: return kind_ == Kind::rgb ? indexed(rgb_to_ansi256(r_, g_, b_)) : *this;
        case ColorProfile::ansi16:
            if (kind_ == Kind::rgb)
            {
                return indexed(rgb_to_ansi16(r_, g_, b_));
            }
            if (kind_ == Kind::indexed)
            {
                return indexed(ansi256_to_ansi16(r_));
            }
            return *this;
    }
    return *this;
}

std::string Color::sgr(bool foreground, ColorProfile profile) const
{
    const Color c = adapt(profile);
    if (!c.is_set())
    {
        return foreground ? "39" : "49";
    }
    if (c.type() == Kind::rgb)
    {
        return (foreground ? std::string{ "38;2;" } : std::string{ "48;2;" }) + std::to_string(c.red()) + ";"
               + std::to_string(c.green()) + ";" + std::to_string(c.blue());
    }
    const int idx = c.index();
    if (profile == ColorProfile::ansi16 && idx < 16)
    {
        const int base = idx < 8 ? (foreground ? 30 : 40) : (foreground ? 90 : 100);
        return std::to_string(base + (idx % 8));
    }
    return (foreground ? std::string{ "38;5;" } : std::string{ "48;5;" }) + std::to_string(idx);
}

ColorProfile detect_color_profile()
{
    if (std::getenv("NO_COLOR") != nullptr)
    {
        return ColorProfile::no_color;
    }
    if (env_contains("COLORTERM", "truecolor") || env_contains("COLORTERM", "24bit"))
    {
        return ColorProfile::truecolor;
    }
    if (env_contains("TERM", "256color"))
    {
        return ColorProfile::ansi256;
    }
    const char* term = std::getenv("TERM");
    if (term == nullptr || std::strcmp(term, "dumb") == 0)
    {
        return ColorProfile::no_color;
    }
    return ColorProfile::ansi16;
}

}  // namespace teacup
