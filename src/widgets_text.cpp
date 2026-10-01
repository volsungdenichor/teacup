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
        const std::size_t end = utf8::next_grapheme(text, pos);
        const std::string_view cluster{ text.data() + pos, end - pos };
        pos = end;
        if (cluster == "\n" || cluster == "\r\n")
        {
            flush_line();
            continue;
        }
        const int cluster_width = utf8::grapheme_width(cluster);
        if (cluster == " ")
        {
            flush_word();
            if (width > 0 && line_width + cluster_width > width)
            {
                trim_trailing_spaces(line);
                lines.push_back(line);
                line.clear();
                line_width = 0;
                continue;  // swallow the space at a wrap point
            }
            line += ' ';
            line_width += cluster_width;
            continue;
        }
        if (width > 0 && word_width + cluster_width > width)
        {
            flush_word();  // single word longer than the line: hard break
            lines.push_back(line);
            line.clear();
            line_width = 0;
        }
        word += cluster;
        word_width += cluster_width;
    }
    flush_line();

    return lines;
}

}  // namespace widgets
}  // namespace teacup
