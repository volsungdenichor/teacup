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

struct Tabs
{
    struct State
    {
        int selected = 0;
    };

    class View
    {
    public:
        using RendererFn = std::function<void(const State&, Canvas)>;

        View& labels(std::vector<std::string> value)
        {
            labels_ = std::move(value);
            return *this;
        }

        View& tab_style(const Style& value)
        {
            tab_style_ = value;
            return *this;
        }

        View& selected_style(const Style& value)
        {
            selected_style_ = value;
            return *this;
        }

        View& renderer(RendererFn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        bool update(State& state, const KeyEvent& event) const
        {
            if (labels_.empty())
            {
                return false;
            }
            state.selected = std::clamp(state.selected, 0, static_cast<int>(labels_.size()) - 1);
            if (event.code == Key::left)
            {
                state.selected = (state.selected + static_cast<int>(labels_.size()) - 1) % static_cast<int>(labels_.size());
                return true;
            }
            if (event.code == Key::right)
            {
                state.selected = (state.selected + 1) % static_cast<int>(labels_.size());
                return true;
            }
            return false;
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (event.action != MouseAction::press || event.button != MouseButton::left || !area.contains(event.position))
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

        void render(const State& state, Canvas target) const
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
                target.text(Point{ x, 0 }, clipped, active ? selected_style_ : tab_style_);
                x += static_cast<int>(clipped.size());
            }
        }

    private:
        std::vector<std::string> labels_;
        Style tab_style_;
        Style selected_style_ = Style{}.bold().underline();
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup