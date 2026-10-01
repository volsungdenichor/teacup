#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>

#include "teacup/canvas.hpp"
#include "teacup/event.hpp"
#include "teacup/glyph.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct TextInput
{
    struct State
    {
        GlyphString value;
        std::size_t cursor = 0;  // glyph index
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
            clamp_cursor(state);
            if (event.code == Key::space)
            {
                insert(state, " ");
                return true;
            }
            if (event.code == Key::character && !event.text.empty() && !has(event.modifiers, Modifier::ctrl)
                && !has(event.modifiers, Modifier::alt))
            {
                insert(state, event.text);
                return true;
            }
            switch (event.code)
            {
                case Key::left:
                    if (state.cursor > 0)
                    {
                        --state.cursor;
                    }
                    return true;
                case Key::right:
                    if (state.cursor < state.value.size())
                    {
                        ++state.cursor;
                    }
                    return true;
                case Key::home: state.cursor = 0; return true;
                case Key::end: state.cursor = state.value.size(); return true;
                case Key::backspace:
                    if (state.cursor > 0)
                    {
                        --state.cursor;
                        state.value.erase(state.value.begin() + static_cast<std::ptrdiff_t>(state.cursor));
                    }
                    return true;
                case Key::del:
                    if (state.cursor < state.value.size())
                    {
                        state.value.erase(state.value.begin() + static_cast<std::ptrdiff_t>(state.cursor));
                    }
                    return true;
                default: return false;
            }
        }

        bool update(State& state, const PasteEvent& event) const
        {
            clamp_cursor(state);
            insert(state, event.text);
            return true;
        }

        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (event.action != MouseAction::press || event.button != MouseButton::left || !area.contains(event.position))
            {
                return false;
            }
            const int column = event.position.x - area.left();
            std::size_t position = scroll_start(state, area.width());
            int width = 0;
            while (position < state.value.size() && width < column)
            {
                const int glyph_width = state.value[position].width();
                if (width + glyph_width / 2 >= column)
                {
                    break;
                }
                width += glyph_width;
                ++position;
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
            const int width = target.size().width;
            const std::size_t start = scroll_start(state, width);
            target.fill(" ", text_style_);
            target.text(Point{ 0, 0 }, state.value.substr(start), text_style_);
            const int cursor_x = state.value.substr(start, cursor - start).width();
            if (cursor_x < width)
            {
                const GlyphString under_cursor
                    = cursor < state.value.size() ? state.value.substr(cursor, 1) : GlyphString::from_utf8(" ");
                target.text(Point{ cursor_x, 0 }, under_cursor, cursor_style_);
            }
        }

    private:
        // Index of the first visible glyph so the cursor stays on screen; shared by render and hit-testing.
        static std::size_t scroll_start(const State& state, int width)
        {
            const std::size_t cursor = std::min(state.cursor, state.value.size());
            const int cursor_width = cursor < state.value.size() ? state.value[cursor].width() : 1;
            std::size_t start = 0;
            int visible = state.value.substr(0, cursor).width();
            while (visible + cursor_width > width && start < cursor)
            {
                visible -= state.value[start].width();
                ++start;
            }
            return start;
        }

        static void clamp_cursor(State& state) { state.cursor = std::min(state.cursor, state.value.size()); }

        // Re-segments around the insertion so typed combining marks join the preceding glyph.
        static void insert(State& state, const std::string& text)
        {
            const std::string before = state.value.substr(0, state.cursor).to_utf8() + text;
            state.value = GlyphString::from_utf8(before + state.value.substr(state.cursor).to_utf8());
            state.cursor = std::min(GlyphString::from_utf8(before).size(), state.value.size());
        }

        Style text_style_;
        Style cursor_style_ = Style{}.reverse();
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup