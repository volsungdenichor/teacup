#pragma once

#include <string>
#include <utility>

#include "teacup/canvas.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct Checkbox
{
    struct State
    {
        bool checked = false;
    };

    class View
    {
    public:
        View& checked_style(const Style& value)
        {
            checked_.style = value;
            return *this;
        }

        View& unchecked_style(const Style& value)
        {
            unchecked_.style = value;
            return *this;
        }

        View& glyphs(std::string checked, std::string unchecked)
        {
            checked_.glyph = std::move(checked);
            unchecked_.glyph = std::move(unchecked);
            return *this;
        }

        bool update(State& state, const KeyEvent& event) const
        {
            if (event.code == Key::space || event.code == Key::enter)
            {
                state.checked = !state.checked;
                return true;
            }
            return false;
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (event.action == MouseAction::press && event.button == MouseButton::left && area.contains(event.position))
            {
                state.checked = !state.checked;
                return true;
            }
            return false;
        }

        void render(const State& state, Canvas target) const
        {
            if (target.valid())
            {
                target.text(
                    Point{ 0, 0 },
                    state.checked ? checked_.glyph : unchecked_.glyph,
                    state.checked ? checked_.style : unchecked_.style);
            }
        }

    private:
        struct StateInfo
        {
            std::string glyph;
            Style style;
        };
        StateInfo checked_ = { "[+]", Style{}.fg(Color::hex("#a6e3a1")).bold() };
        StateInfo unchecked_ = { "[ ]", Style{}.fg(Color::hex("#6c7086")) };
    };
};

}  // namespace widgets
}  // namespace teacup