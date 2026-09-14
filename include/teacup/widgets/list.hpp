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
struct basic_list_state
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
template <typename Item>
class basic_list_view
{
public:
    using renderer_fn = std::function<void(const Item&, canvas, const style&, bool selected)>;
    using view_renderer_fn = std::function<void(const basic_list_state<Item>&, canvas)>;

    basic_list_view()
    {
        if constexpr (std::is_same_v<Item, std::string>)
        {
            renderer_ = [](const std::string& item, canvas row, const style& item_style, bool selected) {
                std::string text = (selected ? "> " : "  ") + item;
                while (static_cast<int>(utf8::width(text)) > row.width())
                {
                    text.pop_back();
                }
                while (static_cast<int>(utf8::width(text)) < row.width())
                {
                    text.push_back(' ');
                }
                row.text(0, 0, text, item_style);
            };
        }
    }

    basic_list_view& selected_style(const style& value)
    {
        selected_style_ = value;
        return *this;
    }
    basic_list_view& item_style(const style& value)
    {
        item_style_ = value;
        return *this;
    }

    basic_list_view& item_height(int value)
    {
        item_height_ = (std::max)(1, value);
        return *this;
    }

    basic_list_view& viewport_height(int value)
    {
        viewport_height_ = (std::max)(0, value);
        return *this;
    }

    basic_list_view& item_renderer(renderer_fn renderer)
    {
        renderer_ = std::move(renderer);
        return *this;
    }

    basic_list_view& renderer(view_renderer_fn renderer)
    {
        view_renderer_ = std::move(renderer);
        return *this;
    }

    bool update(basic_list_state<Item>& state, const key_event& event) const
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

    void ensure_visible(basic_list_state<Item>& state, int height) const
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

    void page_down(basic_list_state<Item>& state, int height) const
    {
        state.offset += (std::max)(1, height / item_height_);
        state.clamp_offset((std::max)(1, height / item_height_));
    }

    void page_up(basic_list_state<Item>& state, int height) const
    {
        state.offset -= (std::max)(1, height / item_height_);
        state.clamp_offset((std::max)(1, height / item_height_));
    }

        bool update(basic_list_state<Item>& state, const mouse_event& event, const rect& area) const
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
                    state.offset = (std::min)(state.offset + 1,
                                              (std::max)(0, static_cast<int>(state.items.size()) - area.height / item_height_));
                    return true;
                }
                if (event.action != mouse_action::press || event.button != mouse_button::left)
                {
                    return false;
                }
                const int row = state.offset + (event.position.y - area.y) / item_height_;
            if (row < 0 || row >= static_cast<int>(state.items.size()))
            {
                return false;
            }
            state.selected = row;
            return true;
        }

    void render(const basic_list_state<Item>& state, canvas target) const
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
        const int selected = state.items.empty()
            ? 0
            : std::clamp(state.selected, 0, static_cast<int>(state.items.size()) - 1);
        const int visible_items = (std::max)(1, target.height() / item_height_);
        const int offset = std::clamp(state.offset, 0, (std::max)(0, static_cast<int>(state.items.size()) - visible_items));
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
    void keep_selected_visible(basic_list_state<Item>& state) const
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

using list_state = basic_list_state<std::string>;
using list_view = basic_list_view<std::string>;

/// A compact item list with a single selected index.
class list
{
public:
    list() = default;

    list& items(std::vector<std::string> value)
    {
        items_ = std::move(value);
        if (selected_ >= static_cast<int>(items_.size()))
        {
            selected_ = static_cast<int>(items_.empty()) ? 0 : static_cast<int>(items_.size()) - 1;
        }
        return *this;
    }
    list& selected(int index)
    {
        if (!items_.empty())
        {
            selected_ = std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
        }
        else
        {
            selected_ = 0;
        }
        return *this;
    }
    [[nodiscard]] int selected() const
    {
        return selected_;
    }

    bool update(const key_event& event)
    {
        if (items_.empty())
        {
            return false;
        }
        if (event.code == key::up || event.matches("k"))
        {
            selected_ = (std::max)(0, selected_ - 1);
            return true;
        }
        if (event.code == key::down || event.matches("j"))
        {
            selected_ = (std::min)(static_cast<int>(items_.size()) - 1, selected_ + 1);
            return true;
        }
        if (event.code == key::home)
        {
            selected_ = 0;
            return true;
        }
        if (event.code == key::end)
        {
            selected_ = static_cast<int>(items_.size()) - 1;
            return true;
        }
        return false;
    }

    list& selected_style(const style& s)
    {
        selected_style_ = s;
        return *this;
    }
    list& item_style(const style& s)
    {
        item_style_ = s;
        return *this;
    }
    void render(canvas target) const
    {
        if (!target.valid())
        {
            return;
        }

        const int height = target.height();
        const int width = target.width();
        const int count = static_cast<int>(items_.size());
        if (count == 0 || height <= 0 || width <= 0)
        {
            return;
        }

        for (int row = 0; row < height; ++row)
        {
            const int item_index = row;
            if (item_index >= count)
            {
                target.fill(rect{ 0, row, width, 1 }, " ");
                continue;
            }

            const std::string& item = items_[item_index];
            const bool is_selected = item_index == selected_;
            const style s = is_selected ? selected_style_ : item_style_;
            const std::string prefix = is_selected ? "> " : "  ";
            std::string text = prefix + item;
            if (static_cast<int>(utf8::width(text)) > width)
            {
                while (static_cast<int>(utf8::width(text)) > width)
                {
                    text.pop_back();
                }
            }
            while (static_cast<int>(utf8::width(text)) < width)
            {
                text.push_back(' ');
            }
            target.text(0, row, text, s);
        }
    }

private:
    std::vector<std::string> items_;
    int selected_ = 0;
    style selected_style_ = style{}.fg(color::hex("#f38ba8")).add(attr::bold);
    style item_style_ = style{}.fg(color::hex("#cdd6f4"));
};

}  // namespace widgets
}  // namespace teacup
