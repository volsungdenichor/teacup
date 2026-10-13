#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/glyph.hpp"
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

    Table& headers(std::vector<GlyphString> value)
    {
        headers_ = std::move(value);
        return *this;
    }

    Table& rows(std::vector<std::vector<GlyphString>> value)
    {
        rows_ = std::move(value);
        return *this;
    }

    Table& header_style(const Style& s)
    {
        style_.header_style = s;
        return *this;
    }

    Table& row_style(const Style& s)
    {
        style_.row_style = s;
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
            target.text(Point{ 0, row_index }, pad_to(render_row(headers_, col_widths), width), style_.header_style);
            ++row_index;
        }

        for (const auto& row : rows_)
        {
            if (row_index >= height)
            {
                break;
            }
            target.text(Point{ 0, row_index }, pad_to(render_row(row, col_widths), width), style_.row_style);
            ++row_index;
        }

        while (row_index < height)
        {
            target.fill(Rect{ Point{ 0, row_index }, Size{ width, 1 } }, " ");
            ++row_index;
        }
    }

private:
    static GlyphString pad_to(GlyphString text, int width)
    {
        for (int w = text.width(); w < width; ++w)
        {
            text.push_back(Glyph{});
        }
        return text;
    }

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
        auto update = [&](const std::vector<GlyphString>& values)
        {
            for (std::size_t i = 0; i < values.size(); ++i)
            {
                widths[i] = (std::max)(widths[i], values[i].width());
            }
        };

        update(headers_);
        for (const auto& row : rows_)
        {
            update(row);
        }

        return widths;
    }

    [[nodiscard]] GlyphString render_row(const std::vector<GlyphString>& cells, const std::vector<int>& widths) const
    {
        GlyphString line;
        for (std::size_t i = 0; i < cells.size(); ++i)
        {
            const int max_width = i < widths.size() ? widths[i] : 0;
            if (!line.empty())
            {
                line.push_back(Glyph{});
            }
            line += max_width > 0 ? pad_to(cells[i].truncate(max_width), max_width) : cells[i];
        }
        return line;
    }

    struct WidgetStyle
    {
        Style header_style = Style{}.fg(Color::hex("#f38ba8")).add(Attr::bold);
        Style row_style = Style{}.fg(Color::hex("#cdd6f4"));
    };
    std::vector<GlyphString> headers_;
    std::vector<std::vector<GlyphString>> rows_;
    WidgetStyle style_;
    RendererFn renderer_;
};

}  // namespace widgets
}  // namespace teacup
