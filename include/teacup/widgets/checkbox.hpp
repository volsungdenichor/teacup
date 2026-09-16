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

struct checkbox
{
    struct state_type
    {
        bool checked = false;
    };

    class view_type
    {
    public:
        view_type& checked_style(const style& value)
        {
            checked_style_ = value;
            return *this;
        }
        view_type& unchecked_style(const style& value)
        {
            unchecked_style_ = value;
            return *this;
        }
        view_type& glyphs(std::string checked, std::string unchecked)
        {
            checked_glyph_ = std::move(checked);
            unchecked_glyph_ = std::move(unchecked);
            return *this;
        }
        bool update(state_type& state, const key_event& event) const
        {
            if (event.code == key::space || event.code == key::enter)
            {
                state.checked = !state.checked;
                return true;
            }
            return false;
        }
        bool update(state_type& state, const mouse_event& event, const rect& area) const
        {
            if (event.action == mouse_action::press && event.button == mouse_button::left && area.contains(event.position))
            {
                state.checked = !state.checked;
                return true;
            }
            return false;
        }
        void render(const state_type& state, canvas target) const
        {
            if (target.valid())
            {
                target.text(
                    0,
                    0,
                    state.checked ? checked_glyph_ : unchecked_glyph_,
                    state.checked ? checked_style_ : unchecked_style_);
            }
        }

    private:
        std::string checked_glyph_ = "[+]";
        std::string unchecked_glyph_ = "[ ]";
        style checked_style_ = style{}.fg(color::hex("#a6e3a1")).bold();
        style unchecked_style_ = style{}.fg(color::hex("#6c7086"));
    };
};

}  // namespace widgets
}  // namespace teacup