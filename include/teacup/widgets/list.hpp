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
struct list
{
    struct state_type
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

    /// View and interaction logic for an application-owned list state.
    class view_type
    {
    public:
        using renderer_fn = std::function<void(const Item&, canvas, const style&, bool selected)>;
        using view_renderer_fn = std::function<void(const state_type&, canvas)>;

        view_type()
        {
            if constexpr (std::is_same_v<Item, std::string>)
            {
                renderer_ = [](const std::string& item, canvas row, const style& item_style, bool selected)
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
                    row.text(point{ 0, 0 }, text, item_style);
                };
            }
        }

        view_type& selected_style(const style& value)
        {
            selected_style_ = value;
            return *this;
        }
        view_type& item_style(const style& value)
        {
            item_style_ = value;
            return *this;
        }

        view_type& item_height(int value)
        {
            item_height_ = (std::max)(1, value);
            return *this;
        }

        view_type& viewport_height(int value)
        {
            viewport_height_ = (std::max)(0, value);
            return *this;
        }

        view_type& item_renderer(renderer_fn renderer)
        {
            renderer_ = std::move(renderer);
            return *this;
        }

        view_type& renderer(view_renderer_fn renderer)
        {
            view_renderer_ = std::move(renderer);
            return *this;
        }

        bool update(state_type& state, const key_event& event) const
        {
            state.clamp_selection();
            if (state.items.empty())
            {
                return false;
            }
            if (event.code == key::up || event.matches("k"))
            {
                state.selected = (std::max)(0, state.selected - 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.code == key::down || event.matches("j"))
            {
                state.selected = (std::min)(static_cast<int>(state.items.size()) - 1, state.selected + 1);
                keep_selected_visible(state);
                return true;
            }
            if (event.code == key::home)
            {
                state.selected = 0;
                keep_selected_visible(state);
                return true;
            }
            if (event.code == key::end)
            {
                state.selected = static_cast<int>(state.items.size()) - 1;
                keep_selected_visible(state);
                return true;
            }
            return false;
        }

        void ensure_visible(state_type& state, int height) const
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

        void page_down(state_type& state, int height) const
        {
            state.offset += (std::max)(1, height / item_height_);
            state.clamp_offset((std::max)(1, height / item_height_));
        }

        void page_up(state_type& state, int height) const
        {
            state.offset -= (std::max)(1, height / item_height_);
            state.clamp_offset((std::max)(1, height / item_height_));
        }

        bool update(state_type& state, const mouse_event& event, const rect& area) const
        {
            if (!area.contains(event.position))
            {
                return false;
            }
            if (event.button == mouse_button::wheel_up)
            {
                state.offset = (std::max)(0, state.offset - 1);
                return true;
            }
            if (event.button == mouse_button::wheel_down)
            {
                state.offset = (std::min)(
                    state.offset + 1, (std::max)(0, static_cast<int>(state.items.size()) - area.height() / item_height_));
                return true;
            }
            if (event.action != mouse_action::press || event.button != mouse_button::left)
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

        void render(const state_type& state, canvas target) const
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
                const style& item_style = active ? selected_style_ : item_style_;
                if (!renderer_)
                {
                    continue;
                }
                const canvas row_canvas = target.sub(rect{ 0, row, target.width(), item_height_ });
                target.fill(rect{ 0, row, target.width(), item_height_ }, " ", item_style);
                renderer_(state.items[static_cast<std::size_t>(item_index)], row_canvas, item_style, active);
            }
        }

    private:
        void keep_selected_visible(state_type& state) const
        {
            if (viewport_height_ > 0)
            {
                ensure_visible(state, viewport_height_);
            }
        }

        style selected_style_ = style{}.fg(color::hex("#f38ba8")).add(attr::bold);
        style item_style_ = style{}.fg(color::hex("#cdd6f4"));
        renderer_fn renderer_;
        view_renderer_fn view_renderer_;
        int item_height_ = 1;
        int viewport_height_ = 0;
    };
};

}  // namespace widgets
}  // namespace teacup
