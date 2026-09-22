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
            checked_style_ = value;
            return *this;
        }
        View& unchecked_style(const Style& value)
        {
            unchecked_style_ = value;
            return *this;
        }
        View& glyphs(std::string checked, std::string unchecked)
        {
            checked_glyph_ = std::move(checked);
            unchecked_glyph_ = std::move(unchecked);
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
                    state.checked ? checked_glyph_ : unchecked_glyph_,
                    state.checked ? checked_style_ : unchecked_style_);
            }
        }

    private:
        std::string checked_glyph_ = "[+]";
        std::string unchecked_glyph_ = "[ ]";
        Style checked_style_ = Style{}.fg(Color::hex("#a6e3a1")).bold();
        Style unchecked_style_ = Style{}.fg(Color::hex("#6c7086"));
    };
};

}  // namespace widgets
}  // namespace teacup