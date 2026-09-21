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

struct tabs
{
    struct state_type
    {
        int selected = 0;
    };

    /// View and interaction logic for an application-owned tabs_state.
    /// Left/right cycle labels; `key::tab` is focus navigation owned by the
    /// router and is intentionally not handled here.
    class view_type
    {
    public:
        using renderer_fn = std::function<void(const state_type&, canvas)>;

        view_type& labels(std::vector<std::string> value)
        {
            labels_ = std::move(value);
            return *this;
        }
        view_type& tab_style(const teacup::style& value)
        {
            tab_style_ = value;
            return *this;
        }
        view_type& selected_style(const teacup::style& value)
        {
            selected_style_ = value;
            return *this;
        }

        view_type& renderer(renderer_fn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        bool update(state_type& state, const key_event& event) const
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
            if (event.code == key::right)
            {
                state.selected = (state.selected + 1) % static_cast<int>(labels_.size());
                return true;
            }
            return false;
        }

        bool update(state_type& state, const mouse_event& event, const rect& area) const
        {
            if (event.action != mouse_action::press || event.button != mouse_button::left || !area.contains(event.position))
            {
                return false;
            }
            int x = area.left();
            for (std::size_t i = 0; i < labels_.size(); ++i)
            {
                const std::string label
                    = (static_cast<int>(i) == state.selected) ? "[" + labels_[i] + "]" : " " + labels_[i] + " ";
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

        void render(const state_type& state, canvas target) const
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
                target.text(point{ x, 0 }, clipped, active ? selected_style_ : tab_style_);
                x += static_cast<int>(clipped.size());
            }
        }

    private:
        std::vector<std::string> labels_;
        teacup::style tab_style_;
        teacup::style selected_style_ = teacup::style{}.bold().underline();
        renderer_fn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup