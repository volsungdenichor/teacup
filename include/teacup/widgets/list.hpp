#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct ListItemState
{
    bool selected = false;
};

template <typename Item>
struct List
{
    struct State
    {
        std::vector<Item> items;
        int selected = 0;
        int offset = 0;

        void clamp_selection()
        {
            selected = items.empty() ? 0 : std::clamp(selected, 0, static_cast<int>(items.size()) - 1);
        }

        void clamp_offset(int visible_items)
        {
            offset = std::clamp(offset, 0, (std::max)(0, static_cast<int>(items.size()) - visible_items));
        }
    };

    class View
    {
    public:
        using RendererFn = std::function<void(const Item&, Canvas, const Style&, const ListItemState&)>;
        using ViewRendererFn = std::function<void(const State&, Canvas)>;

        View()
        {
            if constexpr (std::is_same_v<Item, std::string>)
            {
                renderer_ = [](const std::string& item, Canvas row, const Style& item_style, const ListItemState& state)
                {
                    row.fill(" ", item_style);
                    row.text(Point{ 0, 0 }, (state.selected ? "> " : "  ") + item, item_style);
                };
            }
        }

        View& selected_style(const Style& value)
        {
            selected_style_ = value;
            return *this;
        }
        View& item_style(const Style& value)
        {
            item_style_ = value;
            return *this;
        }

        View& item_height(int value)
        {
            item_height_ = (std::max)(1, value);
            return *this;
        }

        View& viewport_height(int value)
        {
            viewport_height_ = (std::max)(0, value);
            return *this;
        }

        View& item_renderer(RendererFn renderer)
        {
            renderer_ = std::move(renderer);
            return *this;
        }

        View& renderer(ViewRendererFn renderer)
        {
            view_renderer_ = std::move(renderer);
            return *this;
        }

        bool update(State& state, const KeyEvent& event) const
        {
            state.clamp_selection();
            if (state.items.empty())
            {
                return false;
            }
            if (event.matches_any({ "up", "k" }))
            {
                state.selected = (std::max)(0, state.selected - 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.matches_any({ "down", "j" }))
            {
                state.selected = (std::min)(static_cast<int>(state.items.size()) - 1, state.selected + 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.matches("home"))
            {
                state.selected = 0;
                keep_selected_visible(state);
                return true;
            }
            if (event.matches("end"))
            {
                state.selected = static_cast<int>(state.items.size()) - 1;
                keep_selected_visible(state);
                return true;
            }
            return false;
        }

        [[nodiscard]] int visible_items(int height) const { return (std::max)(1, height / item_height_); }

        // Index of the first item actually drawn for a given height.
        [[nodiscard]] int first_visible(const State& state, int height) const
        {
            return std::clamp(
                state.offset, 0, (std::max)(0, static_cast<int>(state.items.size()) - visible_items(height)));
        }

        // Item index under `point`, or -1; mirrors the rows painted by render().
        [[nodiscard]] int item_at(const State& state, const Point& point, const Rect& area) const
        {
            if (!area.contains(point))
            {
                return -1;
            }
            const int visible_index = (point.y - area.top()) / item_height_;
            const int index = first_visible(state, area.height()) + visible_index;
            if (visible_index >= visible_items(area.height()) || index >= static_cast<int>(state.items.size()))
            {
                return -1;
            }
            return index;
        }

        void ensure_visible(State& state, int height) const
        {
            const int visible = visible_items(height);
            state.clamp_selection();
            state.clamp_offset(visible);
            if (state.selected < state.offset)
            {
                state.offset = state.selected;
            }
            else if (state.selected >= state.offset + visible)
            {
                state.offset = state.selected - visible + 1;
            }
            state.clamp_offset(visible);
        }

        void page_down(State& state, int height) const { scroll(state, visible_items(height), height); }

        void page_up(State& state, int height) const { scroll(state, -visible_items(height), height); }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (!area.contains(event.position))
            {
                return false;
            }
            if (event.button == MouseButton::wheel_up)
            {
                scroll(state, -1, area.height());
                return true;
            }
            if (event.button == MouseButton::wheel_down)
            {
                scroll(state, 1, area.height());
                return true;
            }
            if (event.action != MouseAction::press || event.button != MouseButton::left)
            {
                return false;
            }
            const int index = item_at(state, event.position, area);
            if (index < 0)
            {
                return false;
            }
            state.selected = index;
            return true;
        }

        void render(const State& state, Canvas target) const
        {
            if (view_renderer_)
            {
                view_renderer_(state, target);
                return;
            }
            if (!target.valid())
            {
                return;
            }
            const int height = target.size().height;
            const int selected
                = state.items.empty() ? 0 : std::clamp(state.selected, 0, static_cast<int>(state.items.size()) - 1);
            const int visible = visible_items(height);
            const int offset = first_visible(state, height);
            for (int visible_index = 0; visible_index < visible; ++visible_index)
            {
                const int item_index = offset + visible_index;
                if (item_index >= static_cast<int>(state.items.size()))
                {
                    break;
                }
                const int row = visible_index * item_height_;
                if (row >= height)
                {
                    break;
                }
                const bool active = item_index == selected;
                const Style& item_style = active ? selected_style_ : item_style_;
                if (!renderer_)
                {
                    continue;
                }
                Canvas row_canvas = target.horizontal_slice(row, item_height_);
                row_canvas.fill(" ", item_style);
                renderer_(
                    state.items[static_cast<std::size_t>(item_index)], row_canvas, item_style, ListItemState{ active });
            }
        }

    private:
        void scroll(State& state, int amount, int height) const
        {
            state.offset = first_visible(state, height) + amount;
            state.clamp_offset(visible_items(height));
        }

        void keep_selected_visible(State& state) const
        {
            if (viewport_height_ > 0)
            {
                ensure_visible(state, viewport_height_);
            }
        }

        Style selected_style_ = Style{}.fg(Color::hex("#f38ba8")).add(Attr::bold);
        Style item_style_ = Style{}.fg(Color::hex("#cdd6f4"));
        RendererFn renderer_;
        ViewRendererFn view_renderer_;
        int item_height_ = 1;
        int viewport_height_ = 0;
    };
};

}  // namespace widgets
}  // namespace teacup
