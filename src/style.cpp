#include "teacup/style.hpp"

namespace teacup
{

std::string sgr_sequence(const Style& s, ColorProfile profile)
{
    std::string out = "\x1b[0";
    const auto append = [&out](const std::string& code)
    {
        out += ';';
        out += code;
    };

    const Attr a = s.attributes();
    if (has(a, Attr::bold))
    {
        append("1");
    }
    if (has(a, Attr::dim))
    {
        append("2");
    }
    if (has(a, Attr::italic))
    {
        append("3");
    }
    if (has(a, Attr::underline))
    {
        append("4");
    }
    if (has(a, Attr::blink))
    {
        append("5");
    }
    if (has(a, Attr::reverse))
    {
        append("7");
    }
    if (has(a, Attr::strikethrough))
    {
        append("9");
    }
    if (profile != ColorProfile::no_color)
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
