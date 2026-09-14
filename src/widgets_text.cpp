#include "teacup/widgets/text.hpp"

namespace teacup
{
namespace widgets
{

std::vector<std::string> wrap_text(const std::string& text, int width)
{
    std::vector<std::string> lines;
    std::string line;
    int line_width = 0;
    std::string word;
    int word_width = 0;

    const auto trim_trailing_spaces = [](std::string& value) {
        while (!value.empty() && value.back() == ' ')
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

    std::size_t pos = 0;
    while (pos < text.size())
    {
        const char32_t cp = utf8::decode(text, pos);
        if (cp == U'\n')
        {
            flush_line();
            continue;
        }
        const int cp_width = utf8::width(cp);
        if (cp == U' ')
        {
            flush_word();
            if (width > 0 && line_width + cp_width > width)
            {
                trim_trailing_spaces(line);
                lines.push_back(line);
                line.clear();
                line_width = 0;
                continue;  // swallow the space at a wrap point
            }
            line += ' ';
            line_width += cp_width;
            continue;
        }
        if (width > 0 && word_width + cp_width > width)
        {
            flush_word();  // single word longer than the line: hard break
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }
        utf8::encode(cp, word);
        word_width += cp_width;
    }
    flush_line();

    return lines;
}

}  // namespace widgets
}  // namespace teacup
