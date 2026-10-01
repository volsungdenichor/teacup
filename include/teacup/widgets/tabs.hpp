#pragma once

#include <algorithm>
#include <functional>
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
            const std::vector<Rect> rects = label_rects(state, area);
            for (std::size_t i = 0; i < rects.size(); ++i)
            {
                if (rects[i].contains(event.position))
                {
                    state.selected = static_cast<int>(i);
                    return true;
                }
            }
            return false;
        }

        // Label rects in `area` coordinates; the single source of truth for render and hit-testing.
        [[nodiscard]] std::vector<Rect> label_rects(const State& state, const Rect& area) const
        {
            std::vector<Rect> rects;
            rects.reserve(labels_.size());
            const int selected = selected_index(state);
            int x = area.left();
            for (std::size_t i = 0; i < labels_.size(); ++i)
            {
                const int width = utf8::width(label_text(i, static_cast<int>(i) == selected));
                rects.emplace_back(x, area.top(), width, 1);
                x += width;
            }
            return rects;
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
            const int selected = selected_index(state);
            const std::vector<Rect> rects = label_rects(state, target.bounds());
            for (std::size_t i = 0; i < rects.size() && rects[i].left() < target.size().width; ++i)
            {
                const bool active = static_cast<int>(i) == selected;
                target.text(rects[i].origin(), label_text(i, active), active ? selected_style_ : tab_style_);
            }
        }

    private:
        [[nodiscard]] int selected_index(const State& state) const
        {
            return labels_.empty() ? 0 : std::clamp(state.selected, 0, static_cast<int>(labels_.size()) - 1);
        }

        [[nodiscard]] std::string label_text(std::size_t index, bool active) const
        {
            return active ? "[" + labels_[index] + "]" : " " + labels_[index] + " ";
        }

        std::vector<std::string> labels_;
        Style tab_style_;
        Style selected_style_ = Style{}.bold().underline();
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup