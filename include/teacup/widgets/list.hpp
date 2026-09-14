#pragma once

#include <algorithm>
#include <string>
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

struct list_state
{
    std::vector<std::string> items;
    int selected = 0;

    void clamp_selection()
    {
        selected = items.empty() ? 0 : std::clamp(selected, 0, static_cast<int>(items.size()) - 1);
    }
};

/// View and interaction logic for an application-owned list_state.
class list_view
{
public:
    list_view& selected_style(const style& value)
    {
        selected_style_ = value;
        return *this;
    }
    list_view& item_style(const style& value)
    {
        item_style_ = value;
        return *this;
    }

    bool update(list_state& state, const key_event& event) const
    {
        state.clamp_selection();
        if (state.items.empty())
            {
                return false;
        }
        if (event.code == key::up || event.matches("k"))
        {
            state.selected = (std::max)(0, state.selected - 1);
            return true;
        }
        if (event.code == key::down || event.matches("j"))
        {
            state.selected = (std::min)(static_cast<int>(state.items.size()) - 1, state.selected + 1);
            return true;
        }
        if (event.code == key::home)
        {
            state.selected = 0;
            return true;
        }
        if (event.code == key::end)
        {
            state.selected = static_cast<int>(state.items.size()) - 1;
            return true;
        }
        return false;
    }

        bool update(list_state& state, const mouse_event& event, const rect& area) const
        {
            if (event.action != mouse_action::press || event.button != mouse_button::left || !area.contains(event.position))
            {
                return false;
            }
            const int row = event.position.y - area.y;
            if (row < 0 || row >= static_cast<int>(state.items.size()))
            {
                return false;
            }
            state.selected = row;
            return true;
        }

    void render(const list_state& state, canvas target) const
    {
        if (!target.valid() || target.width() <= 0 || target.height() <= 0)
        {
            return;
        }
        const int selected = state.items.empty()
            ? 0
            : std::clamp(state.selected, 0, static_cast<int>(state.items.size()) - 1);
        for (int row = 0; row < target.height(); ++row)
        {
            if (row >= static_cast<int>(state.items.size()))
            {
                target.fill(rect{ 0, row, target.width(), 1 }, " ");
                continue;
            }
            const bool active = row == selected;
            std::string text = (active ? "> " : "  ") + state.items[static_cast<std::size_t>(row)];
            while (static_cast<int>(utf8::width(text)) > target.width())
            {
                text.pop_back();
            }
            while (static_cast<int>(utf8::width(text)) < target.width())
            {
                text.push_back(' ');
            }
            target.text(0, row, text, active ? selected_style_ : item_style_);
        }
    }

private:
    style selected_style_ = style{}.fg(color::hex("#f38ba8")).add(attr::bold);
    style item_style_ = style{}.fg(color::hex("#cdd6f4"));
};

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
