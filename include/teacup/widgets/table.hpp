#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

class Table
{
public:
    using RendererFn = std::function<void(Canvas)>;

    Table() = default;

    Table& headers(std::vector<std::string> value)
    {
        headers_ = std::move(value);
        return *this;
    }

    Table& rows(std::vector<std::vector<std::string>> value)
    {
        rows_ = std::move(value);
        return *this;
    }

    Table& header_style(const Style& s)
    {
        header_style_ = s;
        return *this;
    }

    Table& row_style(const Style& s)
    {
        row_style_ = s;
        return *this;
    }

    Table& renderer(RendererFn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    void render(Canvas target) const
    {
        if (renderer_)
        {
            renderer_(target);
            return;
        }
        if (!target.valid())
        {
            return;
        }

        const int width = target.size().width;
        const int height = target.size().height;
        if (width <= 0 || height <= 0)
        {
            return;
        }

        const std::vector<int> col_widths = compute_column_widths();
        int row_index = 0;

        if (!headers_.empty())
        {
            std::string line = render_row(headers_, col_widths);
            while (static_cast<int>(utf8::width(line)) < width)
            {
                line.push_back(' ');
            }
            target.text(Point{ 0, row_index }, line, header_style_);
            ++row_index;
        }

        for (const auto& row : rows_)
        {
            if (row_index >= height)
            {
                break;
            }
            std::string line = render_row(row, col_widths);
            while (static_cast<int>(utf8::width(line)) < width)
            {
                line.push_back(' ');
            }
            target.text(Point{ 0, row_index }, line, row_style_);
            ++row_index;
        }

        while (row_index < height)
        {
            target.fill(Rect{ Point{ 0, row_index }, Size{ width, 1 } }, " ");
            ++row_index;
        }
    }

private:
    [[nodiscard]] std::vector<int> compute_column_widths() const
    {
        std::size_t column_count = 0;
        for (const auto& row : rows_)
        {
            column_count = (std::max)(column_count, row.size());
        }
        column_count = (std::max)(column_count, headers_.size());
        if (column_count == 0)
        {
            return {};
        }

        std::vector<int> widths(column_count, 0);
        auto update = [&](const std::vector<std::string>& values)
        {
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                widths[i] = (std::max)(widths[i], static_cast<int>(utf8::width(values[i])));
            }
        };

        update(headers_);
        for (const auto& row : rows_)
        {
            update(row);
        }

        return widths;
    }

    [[nodiscard]] std::string render_row(const std::vector<std::string>& cells, const std::vector<int>& widths) const
    {
        std::string line;
        for (std::size_t i = 0; i < cells.size(); ++i)
        {
            std::string cell = cells[i];
            const int max_width = i < widths.size() ? widths[i] : 0;
            if (max_width > 0)
            {
                const int current = static_cast<int>(utf8::width(cell));
                if (current > max_width)
                {
                    while (static_cast<int>(utf8::width(cell)) > max_width)
                    {
                        cell.pop_back();
                    }
                }
                else
                {
                    cell.append(static_cast<std::size_t>(max_width - current), ' ');
                }
            }
            if (!line.empty())
            {
                line.push_back(' ');
            }
            line += cell;
        }
        return line;
    }

    std::vector<std::string> headers_;
    std::vector<std::vector<std::string>> rows_;
    Style header_style_ = Style{}.fg(Color::hex("#f38ba8")).add(Attr::bold);
    Style row_style_ = Style{}.fg(Color::hex("#cdd6f4"));
    RendererFn renderer_;
};

}  // namespace widgets
}  // namespace teacup
