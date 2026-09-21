#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

#include "teacup/canvas.hpp"
#include "teacup/detail/utf8.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct text_input
{
    struct state_type
    {
        std::string value;
        std::size_t cursor = 0;
    };

    /// View and editing logic for an application-owned text_input_state.
    class view_type
    {
    public:
        using renderer_fn = std::function<void(const state_type&, canvas)>;

        view_type& text_style(const teacup::style& value)
        {
            text_style_ = value;
            return *this;
        }
        view_type& cursor_style(const teacup::style& value)
        {
            cursor_style_ = value;
            return *this;
        }

        view_type& renderer(renderer_fn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        bool update(state_type& state, const key_event& event) const
        {
            normalize_cursor(state);
            if (event.code == key::space)
            {
                state.value.insert(state.cursor, " ");
                ++state.cursor;
                return true;
            }
            if (event.code == key::character && !event.text.empty() && !has(event.modifiers, modifier::ctrl)
                && !has(event.modifiers, modifier::alt))
            {
                state.value.insert(state.cursor, event.text);
                state.cursor += event.text.size();
                normalize_cursor(state);
                return true;
            }
            switch (event.code)
            {
                case key::left: state.cursor = previous_cursor(state); return true;
                case key::right: state.cursor = next_cursor(state, state.cursor); return true;
                case key::home: state.cursor = 0; return true;
                case key::end: state.cursor = state.value.size(); return true;
                case key::backspace:
                    if (state.cursor > 0)
                    {
                        const std::size_t start = previous_cursor(state);
                        state.value.erase(start, state.cursor - start);
                        state.cursor = start;
                    }
                    return true;
                case key::del:
                    if (state.cursor < state.value.size())
                    {
                        state.value.erase(state.cursor, next_cursor(state, state.cursor) - state.cursor);
                    }
                    return true;
                default: return false;
            }
        }

        bool update(state_type& state, const paste_event& event) const
        {
            normalize_cursor(state);
            state.value.insert(state.cursor, event.text);
            state.cursor += event.text.size();
            normalize_cursor(state);
            return true;
        }

        bool update(state_type& state, const mouse_event& event, const rect& area) const
        {
            if (event.action != mouse_action::press || event.button != mouse_button::left || !area.contains(event.position))
            {
                return false;
            }
            const int column = event.position.x - area.left();
            std::size_t position = 0;
            int width = 0;
            while (position < state.value.size() && width < column)
            {
                const std::size_t next = next_cursor(state, position);
                const int glyph_width = utf8::width(state.value.substr(position, next - position));
                if (width + glyph_width / 2 >= column)
                {
                    break;
                }
                width += glyph_width;
                position = next;
            }
            state.cursor = position;
            return true;
        }

        void render(const state_type& state, canvas target) const
        {
            if (renderer_)
            {
                renderer_(state, target);
                return;
            }
            if (!target.valid() || target.width() <= 0)
            {
                return;
            }
            const std::size_t cursor = std::min(state.cursor, state.value.size());
            const int cursor_width = cursor < state.value.size()
                                         ? utf8::width(state.value.substr(cursor, next_cursor(state, cursor) - cursor))
                                         : 1;
            std::size_t start = 0;
            while (utf8::width(state.value.substr(start, cursor - start)) + cursor_width > target.width() && start < cursor)
            {
                start = next_cursor(state, start);
            }
            target.fill(" ", text_style_);
            target.text(point{ 0, 0 }, state.value.substr(start), text_style_);
            const int cursor_x = utf8::width(state.value.substr(start, cursor - start));
            if (cursor_x < target.width())
            {
                const std::string glyph
                    = cursor < state.value.size() ? state.value.substr(cursor, next_cursor(state, cursor) - cursor) : " ";
                target.text(point{ cursor_x, 0 }, glyph, cursor_style_);
            }
        }

    private:
        static void normalize_cursor(state_type& state)
        {
            state.cursor = std::min(state.cursor, state.value.size());
            while (state.cursor > 0 && state.cursor < state.value.size()
                   && (static_cast<unsigned char>(state.value[state.cursor]) & 0xC0u) == 0x80u)
            {
                --state.cursor;
            }
        }
        static std::size_t previous_cursor(const state_type& state)
        {
            if (state.cursor == 0)
                return 0;
            std::size_t position = state.cursor - 1;
            while (position > 0 && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u)
                --position;
            return position;
        }
        static std::size_t next_cursor(const state_type& state, std::size_t position)
        {
            if (position >= state.value.size())
                return state.value.size();
            ++position;
            while (position < state.value.size() && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u)
                ++position;
            return position;
        }

        teacup::style text_style_;
        teacup::style cursor_style_ = teacup::style{}.reverse();
        renderer_fn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup