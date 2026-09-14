#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

/// A horizontally arranged, keyboard-selectable tab bar.
class tabs
{
public:
    tabs() = default;

    tabs& labels(std::vector<std::string> value) &
    {
        labels_ = std::move(value);
        clamp_selected();
        return *this;
    }
    tabs&& labels(std::vector<std::string> value) &&
    {
        return std::move(this->labels(std::move(value)));
    }

    tabs& selected(int value) &
    {
        selected_ = value;
        clamp_selected();
        return *this;
    }
    tabs&& selected(int value) &&
    {
        return std::move(this->selected(value));
    }

    tabs& tab_style(const teacup::style& value) &
    {
        tab_style_ = value;
        return *this;
    }
    tabs&& tab_style(const teacup::style& value) &&
    {
        return std::move(this->tab_style(value));
    }

    tabs& selected_style(const teacup::style& value) &
    {
        selected_style_ = value;
        return *this;
    }
    tabs&& selected_style(const teacup::style& value) &&
    {
        return std::move(this->selected_style(value));
    }

    [[nodiscard]] int selected() const
    {
        return selected_;
    }

    bool update(const key_event& event)
    {
        if (labels_.empty())
        {
            return false;
        }
        if (event.code == key::left)
        {
            selected_ = (selected_ + static_cast<int>(labels_.size()) - 1)
                % static_cast<int>(labels_.size());
            return true;
        }
        if (event.code == key::right || event.code == key::tab || event.matches("tab"))
        {
            selected_ = (selected_ + 1) % static_cast<int>(labels_.size());
            return true;
        }
        return false;
    }

    void render(canvas target) const
    {
        if (!target.valid())
        {
            return;
        }
        target.fill(" ", tab_style_);
        int x = 0;
        for (std::size_t i = 0; i < labels_.size() && x < target.width(); ++i)
        {
            const bool selected = i == static_cast<std::size_t>(selected_);
            const std::string text = selected ? "[" + labels_[i] + "]" : " " + labels_[i] + " ";
            const int remaining = target.width() - x;
            const std::string clipped = text.substr(0, static_cast<std::size_t>(remaining));
            target.text(x, 0, clipped, selected ? selected_style_ : tab_style_);
            x += static_cast<int>(clipped.size());
        }
    }

private:
    void clamp_selected()
    {
        if (labels_.empty())
        {
            selected_ = 0;
        }
        else
        {
            selected_ = std::clamp(selected_, 0, static_cast<int>(labels_.size()) - 1);
        }
    }

    std::vector<std::string> labels_;
    int selected_ = 0;
    teacup::style tab_style_;
    teacup::style selected_style_ = teacup::style{}.bold().underline();
};

}  // namespace widgets
}  // namespace teacup