#pragma once

#include <limits>
#include <stdexcept>

#include "teacup/border.hpp"
#include "teacup/widgets/drawing_area.hpp"

namespace teacup
{
namespace widgets
{

/// A selectable map of square-pixel tiles, rendered using two colors per terminal cell.
struct TileMap
{
    struct State
    {
        Point selected{};
        DrawingArea::State drawing;
    };

    class View
    {
    public:
        /// The second argument is a tile-local pixel, with twice as many rows as terminal cells.
        using PixelFn = std::function<Color(Point tile, Point pixel)>;

        View& map_size(Size value)
        {
            validate(value, tile_size_);
            map_size_ = value;
            return *this;
        }

        View& tile_size(Size value)
        {
            if (value.width < 3 || value.height < 3)
            {
                throw std::invalid_argument("TileMap tiles must be at least 3 columns by 3 rows");
            }
            validate(map_size_, value);
            tile_size_ = value;
            return *this;
        }

        View& viewport_size(Size value)
        {
            viewport_ = value;
            return *this;
        }

        View& pixel(PixelFn value)
        {
            pixel_ = std::move(value);
            return *this;
        }

        View& background(Style value)
        {
            background_ = std::move(value);
            return *this;
        }

        View& selected_style(Style value)
        {
            selected_style_ = std::move(value);
            return *this;
        }

        [[nodiscard]] Size map_size() const { return map_size_; }
        [[nodiscard]] Size tile_size() const { return tile_size_; }

        [[nodiscard]] Rect tile_rect(Point tile) const
        {
            if (!Rect::from_size(map_size_).contains(tile))
            {
                throw std::out_of_range("TileMap tile coordinates are outside the map");
            }
            return Rect{ tile.x * tile_size_.width, tile.y * tile_size_.height, tile_size_.width, tile_size_.height };
        }

        [[nodiscard]] std::optional<Point> tile_at(const State& state, Point screen, Rect area) const
        {
            const auto content = drawing_view(area.size()).to_content(state.drawing, screen, area);
            if (!content)
            {
                return std::nullopt;
            }
            return Point{ content->x / tile_size_.width, content->y / tile_size_.height };
        }

        void ensure_visible(State& state) const
        {
            if (!map_size_.empty())
            {
                state.selected = clamp_tile(state.selected);
                drawing_view(viewport_).ensure_visible(state.drawing, tile_rect(state.selected));
            }
        }

        bool update(State& state, const KeyEvent& event) const
        {
            if (map_size_.empty())
            {
                return false;
            }
            Point next = clamp_tile(state.selected);
            if (event.matches_any({ "up", "k" }))
            {
                --next.y;
            }
            else if (event.matches_any({ "down", "j" }))
            {
                ++next.y;
            }
            else if (event.matches_any({ "left", "h" }))
            {
                --next.x;
            }
            else if (event.matches_any({ "right", "l" }))
            {
                ++next.x;
            }
            else
            {
                return drawing_view(viewport_).update(state.drawing, event);
            }

            const Point before = state.selected;
            const Point offset = state.drawing.offset;
            state.selected = clamp_tile(next);
            ensure_visible(state);
            return state.selected != before || state.drawing.offset != offset;
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (event.action == MouseAction::press && event.button == MouseButton::left)
            {
                const auto tile = tile_at(state, event.position, area);
                if (!tile)
                {
                    return false;
                }
                const Point before = state.selected;
                const Point offset = state.drawing.offset;
                state.selected = *tile;
                drawing_view(area.size()).ensure_visible(state.drawing, tile_rect(*tile));
                return state.selected != before || state.drawing.offset != offset;
            }
            return drawing_view(area.size()).update(state.drawing, event, area);
        }

        void render(const State& state, Canvas target) const
        {
            target.fill(" ", background_);
            if (!target.valid() || map_size_.empty())
            {
                return;
            }
            const auto drawing = drawing_view(target.size());
            const Point offset = drawing.visible_offset(state.drawing, target.size());
            const Rect visible = drawing.visible_rect(state.drawing, target.size());
            const Rect selected = tile_rect(clamp_tile(state.selected));
            const Border border = Border::plain();
            // Render only visible cells; a large world does not need a world-sized Surface.
            visible.for_each(
                [&](Point p)
                {
                    const Point tile{ p.x / tile_size_.width, p.y / tile_size_.height };
                    const Point local{ p.x % tile_size_.width, 2 * (p.y % tile_size_.height) };
                    Style style = background_;
                    Glyph glyph{ " " };
                    if (pixel_)
                    {
                        style.fg(pixel_(tile, local)).bg(pixel_(tile, Point{ local.x, local.y + 1 }));
                        glyph = "▀";
                    }
                    if (selected.contains(p))
                    {
                        const bool left = p.x == selected.left();
                        const bool right = p.x == selected.right() - 1;
                        const bool top = p.y == selected.top();
                        const bool bottom = p.y == selected.bottom() - 1;
                        if (left || right || top || bottom)
                        {
                            glyph = top      ? (left ? border.top_left : right ? border.top_right : border.top)
                                    : bottom ? (left ? border.bottom_left : right ? border.bottom_right : border.bottom)
                                    : left   ? border.left
                                             : border.right;
                            style.merge(selected_style_);
                        }
                    }
                    target.put(p - offset, Cell{ glyph, style });
                });
        }

    private:
        static void validate(Size map, Size tile)
        {
            if (map.width < 0 || map.height < 0 || map.width > (std::numeric_limits<int>::max)() / tile.width
                || map.height > (std::numeric_limits<int>::max)() / tile.height
                || tile.height > (std::numeric_limits<int>::max)() / 2)
            {
                throw std::invalid_argument("TileMap dimensions are negative or exceed supported coordinates");
            }
        }

        Point clamp_tile(Point p) const
        {
            return Point{ std::clamp(p.x, 0, map_size_.width - 1), std::clamp(p.y, 0, map_size_.height - 1) };
        }

        DrawingArea::View drawing_view(Size viewport) const
        {
            return DrawingArea::View{}
                .content_size(Size{ map_size_.width * tile_size_.width, map_size_.height * tile_size_.height })
                .viewport_size(viewport);
        }

        Size map_size_{};
        Size tile_size_{ 6, 3 };
        Size viewport_{};
        PixelFn pixel_;
        Style background_ = Style{}.bg(Color::hex("#182322"));
        Style selected_style_ = Style{}.fg(Color::hex("#ffdf87")).bg(Color::hex("#273130")).bold();
    };
};

}  // namespace widgets
}  // namespace teacup
