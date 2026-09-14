#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace utf8
{
namespace
{

struct range
{
    char32_t first;
    char32_t last;
};

// Zero-width: combining marks and variation selectors (compact subset).
const range zero_width[] = {
    { 0x0300, 0x036F }, { 0x0483, 0x0489 }, { 0x0591, 0x05BD }, { 0x0610, 0x061A },
    { 0x064B, 0x065F }, { 0x0670, 0x0670 }, { 0x06D6, 0x06DC }, { 0x0730, 0x074A },
    { 0x07EB, 0x07F3 }, { 0x0816, 0x0819 }, { 0x0A01, 0x0A03 }, { 0x0E31, 0x0E31 },
    { 0x0E34, 0x0E3A }, { 0x200B, 0x200F }, { 0x20D0, 0x20F0 }, { 0xFE00, 0xFE0F },
    { 0xFE20, 0xFE2F }, { 0x1AB0, 0x1AFF }, { 0x1DC0, 0x1DFF },
};

// Wide (East Asian W/F) and emoji presentation ranges (compact subset).
const range wide[] = {
    { 0x1100, 0x115F }, { 0x2329, 0x232A }, { 0x2E80, 0x303E }, { 0x3041, 0x33FF },
    { 0x3400, 0x4DBF }, { 0x4E00, 0x9FFF }, { 0xA000, 0xA4CF }, { 0xA960, 0xA97F },
    { 0xAC00, 0xD7A3 }, { 0xF900, 0xFAFF }, { 0xFE10, 0xFE19 }, { 0xFE30, 0xFE6F },
    { 0xFF00, 0xFF60 }, { 0xFFE0, 0xFFE6 }, { 0x1F004, 0x1F004 }, { 0x1F0CF, 0x1F0CF },
    { 0x1F300, 0x1F64F }, { 0x1F680, 0x1F6FF }, { 0x1F900, 0x1F9FF },
    { 0x20000, 0x2FFFD }, { 0x30000, 0x3FFFD },
};

template <std::size_t N>
bool in_table(const range (&table)[N], char32_t cp)
{
    std::size_t lo = 0;
    std::size_t hi = N;
    while (lo < hi)
    {
        const std::size_t mid = (lo + hi) / 2;
        if (cp < table[mid].first)
        {
            hi = mid;
        }
        else if (cp > table[mid].last)
        {
            lo = mid + 1;
        }
        else
        {
            return true;
        }
    }
    return false;
}

}  // namespace

char32_t decode(const std::string& text, std::size_t& pos)
{
    if (pos >= text.size())
    {
        return 0;
    }
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    const unsigned char lead = byte(pos);
    std::size_t extra = 0;
    char32_t cp = 0;
    if (lead < 0x80)
    {
        ++pos;
        return lead;
    }
    else if ((lead & 0xE0) == 0xC0)
    {
        extra = 1;
        cp = lead & 0x1Fu;
    }
    else if ((lead & 0xF0) == 0xE0)
    {
        extra = 2;
        cp = lead & 0x0Fu;
    }
    else if ((lead & 0xF8) == 0xF0)
    {
        extra = 3;
        cp = lead & 0x07u;
    }
    else
    {
        ++pos;
        return 0xFFFD;
    }

    if (pos + extra >= text.size())
    {
        ++pos;
        return 0xFFFD;
    }
    for (std::size_t i = 1; i <= extra; ++i)
    {
        const unsigned char cont = byte(pos + i);
        if ((cont & 0xC0) != 0x80)
        {
            ++pos;
            return 0xFFFD;
        }
        cp = (cp << 6) | (cont & 0x3Fu);
    }
    pos += extra + 1;
    return cp;
}

void encode(char32_t cp, std::string& out)
{
    if (cp < 0x80)
    {
        out += static_cast<char>(cp);
    }
    else if (cp < 0x800)
    {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    else if (cp < 0x10000)
    {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
    else
    {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

int width(char32_t cp)
{
    if (cp == 0)
    {
        return 0;
    }
    if (cp < 32 || (cp >= 0x7F && cp < 0xA0))
    {
        return 0;
    }
    if (in_table(zero_width, cp))
    {
        return 0;
    }
    if (in_table(wide, cp))
    {
        return 2;
    }
    return 1;
}

int width(const std::string& text)
{
    int total = 0;
    std::size_t pos = 0;
    while (pos < text.size())
    {
        total += width(decode(text, pos));
    }
    return total;
}

std::vector<char32_t> to_code_points(const std::string& text)
{
    std::vector<char32_t> result;
    std::size_t pos = 0;
    while (pos < text.size())
    {
        result.push_back(decode(text, pos));
    }
    return result;
}

}  // namespace utf8
}  // namespace teacup
