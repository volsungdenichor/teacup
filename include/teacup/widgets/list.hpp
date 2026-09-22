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
        using RendererFn = std::function<void(const Item&, Canvas, const Style&, bool selected)>;
        using ViewRendererFn = std::function<void(const State&, Canvas)>;

        View()
        {
            if constexpr (std::is_same_v<Item, std::string>)
            {
                renderer_ = [](const std::string& item, Canvas row, const Style& item_style, bool selected)
                {
                    std::string text = (selected ? "> " : "  ") + item;
                    while (static_cast<int>(utf8::width(text)) > row.width())
                    {
                        text.pop_back();
                    }
                    while (static_cast<int>(utf8::width(text)) < row.width())
                    {
                        text.push_back(' ');
                    }
                    row.text(Point{ 0, 0 }, text, item_style);
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
            if (event.code == Key::up || event.matches("k"))
            {
                state.selected = (std::max)(0, state.selected - 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.code == Key::down || event.matches("j"))
            {
                state.selected = (std::min)(static_cast<int>(state.items.size()) - 1, state.selected + 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.code == Key::home)
            {
                state.selected = 0;
                keep_selected_visible(state);
                return true;
            }
            if (event.code == Key::end)
            {
                state.selected = static_cast<int>(state.items.size()) - 1;
                keep_selected_visible(state);
                return true;
            }
            return false;
        }

        void ensure_visible(State& state, int height) const
        {
            const int visible_items = (std::max)(1, height / item_height_);
            state.clamp_selection();
            state.clamp_offset(visible_items);
            if (state.selected < state.offset)
            {
                state.offset = state.selected;
            }
            else if (state.selected >= state.offset + visible_items)
            {
                state.offset = state.selected - visible_items + 1;
            }
            state.clamp_offset(visible_items);
        }

        void page_down(State& state, int height) const
        {
            state.offset += (std::max)(1, height / item_height_);
            state.clamp_offset((std::max)(1, height / item_height_));
        }

        void page_up(State& state, int height) const
        {
            state.offset -= (std::max)(1, height / item_height_);
            state.clamp_offset((std::max)(1, height / item_height_));
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (!area.contains(event.position))
            {
                return false;
            }
            if (event.button == MouseButton::wheel_up)
            {
                state.offset = (std::max)(0, state.offset - 1);
                return true;
            }
            if (event.button == MouseButton::wheel_down)
            {
                state.offset = (std::min)(
                    state.offset + 1, (std::max)(0, static_cast<int>(state.items.size()) - area.height() / item_height_));
                return true;
            }
            if (event.action != MouseAction::press || event.button != MouseButton::left)
            {
                return false;
            }
            const int row = state.offset + (event.position.y - area.top()) / item_height_;
            if (row < 0 || row >= static_cast<int>(state.items.size()))
            {
                return false;
            }
            state.selected = row;
            return true;
        }

        void render(const State& state, Canvas target) const
        {
            if (view_renderer_)
            {
                view_renderer_(state, target);
                return;
            }
            if (!target.valid() || target.width() <= 0 || target.height() <= 0)
            {
                return;
            }
            const int selected
                = state.items.empty() ? 0 : std::clamp(state.selected, 0, static_cast<int>(state.items.size()) - 1);
            const int visible_items = (std::max)(1, target.height() / item_height_);
            const int offset
                = std::clamp(state.offset, 0, (std::max)(0, static_cast<int>(state.items.size()) - visible_items));
            for (int visible_index = 0; visible_index < visible_items; ++visible_index)
            {
                const int item_index = offset + visible_index;
                if (item_index >= static_cast<int>(state.items.size()))
                {
                    break;
                }
                const int row = visible_index * item_height_;
                if (row >= target.height())
                {
                    break;
                }
                const bool active = item_index == selected;
                const Style& item_style = active ? selected_style_ : item_style_;
                if (!renderer_)
                {
                    continue;
                }
                const Canvas row_canvas = target.sub(Rect{ 0, row, target.width(), item_height_ });
                target.fill(Rect{ 0, row, target.width(), item_height_ }, " ", item_style);
                renderer_(state.items[static_cast<std::size_t>(item_index)], row_canvas, item_style, active);
            }
        }

    private:
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
