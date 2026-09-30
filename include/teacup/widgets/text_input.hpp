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

struct TextInput
{
    struct State
    {
        std::string value;
        std::size_t cursor = 0;
    };

    class View
    {
    public:
        using RendererFn = std::function<void(const State&, Canvas)>;

        View& text_style(const Style& value)
        {
            text_style_ = value;
            return *this;
        }
        View& cursor_style(const Style& value)
        {
            cursor_style_ = value;
            return *this;
        }

        View& renderer(RendererFn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        bool update(State& state, const KeyEvent& event) const
        {
            normalize_cursor(state);
            if (event.code == Key::space)
            {
                state.value.insert(state.cursor, " ");
                ++state.cursor;
                return true;
            }
            if (event.code == Key::character && !event.text.empty() && !has(event.modifiers, Modifier::ctrl)
                && !has(event.modifiers, Modifier::alt))
            {
                state.value.insert(state.cursor, event.text);
                state.cursor += event.text.size();
                normalize_cursor(state);
                return true;
            }
            switch (event.code)
            {
                case Key::left: state.cursor = previous_cursor(state); return true;
                case Key::right: state.cursor = next_cursor(state, state.cursor); return true;
                case Key::home: state.cursor = 0; return true;
                case Key::end: state.cursor = state.value.size(); return true;
                case Key::backspace:
                    if (state.cursor > 0)
                    {
                        const std::size_t start = previous_cursor(state);
                        state.value.erase(start, state.cursor - start);
                        state.cursor = start;
                    }
                    return true;
                case Key::del:
                    if (state.cursor < state.value.size())
                    {
                        state.value.erase(state.cursor, next_cursor(state, state.cursor) - state.cursor);
                    }
                    return true;
                default: return false;
            }
        }

        bool update(State& state, const PasteEvent& event) const
        {
            normalize_cursor(state);
            state.value.insert(state.cursor, event.text);
            state.cursor += event.text.size();
            normalize_cursor(state);
            return true;
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (event.action != MouseAction::press || event.button != MouseButton::left || !area.contains(event.position))
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
            const std::size_t cursor = std::min(state.cursor, state.value.size());
            const int cursor_width = cursor < state.value.size()
                                         ? utf8::width(state.value.substr(cursor, next_cursor(state, cursor) - cursor))
                                         : 1;
            std::size_t start = 0;
            const int width = target.size().width;
            while (utf8::width(state.value.substr(start, cursor - start)) + cursor_width > width && start < cursor)
            {
                start = next_cursor(state, start);
            }
            target.fill(" ", text_style_);
            target.text(Point{ 0, 0 }, state.value.substr(start), text_style_);
            const int cursor_x = utf8::width(state.value.substr(start, cursor - start));
            if (cursor_x < width)
            {
                const std::string glyph
                    = cursor < state.value.size() ? state.value.substr(cursor, next_cursor(state, cursor) - cursor) : " ";
                target.text(Point{ cursor_x, 0 }, glyph, cursor_style_);
            }
        }

    private:
        static void normalize_cursor(State& state)
        {
            state.cursor = std::min(state.cursor, state.value.size());
            while (state.cursor > 0 && state.cursor < state.value.size()
                   && (static_cast<unsigned char>(state.value[state.cursor]) & 0xC0u) == 0x80u)
            {
                --state.cursor;
            }
        }
        static std::size_t previous_cursor(const State& state)
        {
            if (state.cursor == 0)
                return 0;
            std::size_t position = state.cursor - 1;
            while (position > 0 && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u)
                --position;
            return position;
        }
        static std::size_t next_cursor(const State& state, std::size_t position)
        {
            if (position >= state.value.size())
                return state.value.size();
            ++position;
            while (position < state.value.size() && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u)
                ++position;
            return position;
        }

        Style text_style_;
        Style cursor_style_ = Style{}.reverse();
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup