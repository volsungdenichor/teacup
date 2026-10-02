#include "teacup/widgets/text.hpp"

namespace teacup
{
namespace widgets
{

std::vector<GlyphString> wrap_text(const GlyphString& text, int width)
{
    const Glyph space{};
    std::vector<GlyphString> lines;
    GlyphString line;
    int line_width = 0;
    GlyphString word;
    int word_width = 0;

    const auto trim_trailing_spaces = [&](GlyphString& value) {
        while (!value.empty() && value.back() == space)
        {
            value.pop_back();
        }
    };

    const auto flush_word = [&] {
        if (word.empty())
        {
            return;
        }
        if (width > 0 && line_width > 0 && line_width + word_width > width)
        {
            trim_trailing_spaces(line);
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }
        line += word;
        line_width += word_width;
        word.clear();
        word_width = 0;
    };

    const auto flush_line = [&] {
        flush_word();
        trim_trailing_spaces(line);
        lines.push_back(line);
        line.clear();
        line_width = 0;
    };

    for (const Glyph& glyph : text)
    {
        if (glyph.utf8() == "\n" || glyph.utf8() == "\r\n")
        {
            flush_line();
            continue;
        }
        const int glyph_width = glyph.width();
        if (glyph == space)
        {
            flush_word();
            if (width > 0 && line_width + glyph_width > width)
            {
                trim_trailing_spaces(line);
                lines.push_back(line);
                line.clear();
                line_width = 0;
                continue;  // swallow the space at a wrap point
            }
            line.push_back(space);
            line_width += glyph_width;
            continue;
        }
        if (width > 0 && word_width + glyph_width > width)
        {
            flush_word();  // single word longer than the line: hard break
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }
        word.push_back(glyph);
        word_width += glyph_width;
    }
    flush_line();

    return lines;
}

}  // namespace widgets
}  // namespace teacup
