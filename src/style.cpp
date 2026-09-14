#include "teacup/style.hpp"

namespace teacup
{

std::string sgr_sequence(const style& s, color_profile profile)
{
    std::string out = "\x1b[0";
    const auto append = [&out](const std::string& code) {
        out += ';';
        out += code;
    };

    const attr a = s.attributes();
    if (has(a, attr::bold))
    {
        append("1");
    }
    if (has(a, attr::dim))
    {
        append("2");
    }
    if (has(a, attr::italic))
    {
        append("3");
    }
    if (has(a, attr::underline))
    {
        append("4");
    }
    if (has(a, attr::blink))
    {
        append("5");
    }
    if (has(a, attr::reverse))
    {
        append("7");
    }
    if (has(a, attr::strikethrough))
    {
        append("9");
    }
    if (profile != color_profile::no_color)
    {
        if (s.foreground().is_set())
        {
            append(s.foreground().sgr(true, profile));
        }
        if (s.background().is_set())
        {
            append(s.background().sgr(false, profile));
        }
    }
    out += 'm';
    return out;
}

}  // namespace teacup
