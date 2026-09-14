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

/// A compact item list with a single selected index.
class list
{
public:
    list() = default;

    list& items(std::vector<std::string> value) &
    {
        items_ = std::move(value);
        if (selected_ >= static_cast<int>(items_.size()))
        {
            selected_ = static_cast<int>(items_.empty()) ? 0 : static_cast<int>(items_.size()) - 1;
        }
        return *this;
    }
    list&& items(std::vector<std::string> value) &&
    {
        return std::move(this->items(std::move(value)));
    }

    list& selected(int index) &
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
    list&& selected(int index) &&
    {
        return std::move(this->selected(index));
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

    list& selected_style(const style& s) &
    {
        selected_style_ = s;
        return *this;
    }
    list&& selected_style(const style& s) &&
    {
        return std::move(this->selected_style(s));
    }

    list& item_style(const style& s) &
    {
        item_style_ = s;
        return *this;
    }
    list&& item_style(const style& s) &&
    {
        return std::move(this->item_style(s));
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
