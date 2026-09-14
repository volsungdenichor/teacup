#pragma once

#include <algorithm>
#include <functional>
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

struct tabs_state
{
    int selected = 0;
};

/// View and interaction logic for an application-owned tabs_state.
class tabs_view
{
public:
    using renderer_fn = std::function<void(const tabs_state&, canvas)>;

    tabs_view& labels(std::vector<std::string> value)
    {
        labels_ = std::move(value);
        return *this;
    }
    tabs_view& tab_style(const teacup::style& value)
    {
        tab_style_ = value;
        return *this;
    }
    tabs_view& selected_style(const teacup::style& value)
    {
        selected_style_ = value;
        return *this;
    }

    tabs_view& renderer(renderer_fn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    bool update(tabs_state& state, const key_event& event) const
    {
        if (labels_.empty())
        {
            return false;
        }
        state.selected = std::clamp(state.selected, 0, static_cast<int>(labels_.size()) - 1);
        if (event.code == key::left)
        {
            state.selected = (state.selected + static_cast<int>(labels_.size()) - 1) % static_cast<int>(labels_.size());
            return true;
        }
        if (event.code == key::right || event.code == key::tab || event.matches("tab"))
        {
            state.selected = (state.selected + 1) % static_cast<int>(labels_.size());
            return true;
        }
        return false;
    }

    bool update(tabs_state& state, const mouse_event& event, const rect& area) const
    {
        if (event.action != mouse_action::press || event.button != mouse_button::left || !area.contains(event.position))
        {
            return false;
        }
        int x = area.x;
        for (std::size_t i = 0; i < labels_.size(); ++i)
        {
            const std::string label = (static_cast<int>(i) == state.selected)
                ? "[" + labels_[i] + "]"
                : " " + labels_[i] + " ";
            const int width = static_cast<int>(label.size());
            if (event.position.x >= x && event.position.x < x + width)
            {
                state.selected = static_cast<int>(i);
                return true;
            }
            x += width;
        }
        return false;
    }

    void render(const tabs_state& state, canvas target) const
    {
        if (renderer_)
        {
            renderer_(state, target);
            return;
        }
        if (!target.valid())
        {
            return;
        }
        target.fill(" ", tab_style_);
        int x = 0;
        const int selected = labels_.empty() ? 0 : std::clamp(state.selected, 0, static_cast<int>(labels_.size()) - 1);
        for (std::size_t i = 0; i < labels_.size() && x < target.width(); ++i)
        {
            const bool active = static_cast<int>(i) == selected;
            const std::string text = active ? "[" + labels_[i] + "]" : " " + labels_[i] + " ";
            const std::string clipped = text.substr(0, static_cast<std::size_t>(target.width() - x));
            target.text(x, 0, clipped, active ? selected_style_ : tab_style_);
            x += static_cast<int>(clipped.size());
        }
    }

private:
    std::vector<std::string> labels_;
    teacup::style tab_style_;
    teacup::style selected_style_ = teacup::style{}.bold().underline();
    renderer_fn renderer_;
};

/// A horizontally arranged, keyboard-selectable tab bar.
class tabs
{
public:
    tabs() = default;

    tabs& labels(std::vector<std::string> value)
    {
        labels_ = std::move(value);
        clamp_selected();
        return *this;
    }
    tabs& selected(int value)
    {
        selected_ = value;
        clamp_selected();
        return *this;
    }
    tabs& tab_style(const teacup::style& value)
    {
        tab_style_ = value;
        return *this;
    }
    tabs& selected_style(const teacup::style& value)
    {
        selected_style_ = value;
        return *this;
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