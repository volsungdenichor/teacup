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

struct text_input_state
{
    std::string value;
    std::size_t cursor = 0;
};

/// View and editing logic for an application-owned text_input_state.
class text_input_view
{
public:
    using renderer_fn = std::function<void(const text_input_state&, canvas)>;

    text_input_view& text_style(const teacup::style& value)
    {
        text_style_ = value;
        return *this;
    }
    text_input_view& cursor_style(const teacup::style& value)
    {
        cursor_style_ = value;
        return *this;
    }

    text_input_view& renderer(renderer_fn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    bool update(text_input_state& state, const key_event& event) const
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

    bool update(text_input_state& state, const paste_event& event) const
    {
        normalize_cursor(state);
        state.value.insert(state.cursor, event.text);
        state.cursor += event.text.size();
        normalize_cursor(state);
        return true;
    }

    bool update(text_input_state& state, const mouse_event& event, const rect& area) const
    {
        if (event.action != mouse_action::press || event.button != mouse_button::left || !area.contains(event.position))
        {
            return false;
        }
        const int column = event.position.x - area.x;
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

    void render(const text_input_state& state, canvas target) const
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
        target.text(0, 0, state.value.substr(start), text_style_);
        const int cursor_x = utf8::width(state.value.substr(start, cursor - start));
        if (cursor_x < target.width())
        {
            const std::string glyph = cursor < state.value.size()
                ? state.value.substr(cursor, next_cursor(state, cursor) - cursor)
                : " ";
            target.text(cursor_x, 0, glyph, cursor_style_);
        }
    }

private:
    static void normalize_cursor(text_input_state& state)
    {
        state.cursor = std::min(state.cursor, state.value.size());
        while (state.cursor > 0 && state.cursor < state.value.size()
               && (static_cast<unsigned char>(state.value[state.cursor]) & 0xC0u) == 0x80u)
        {
            --state.cursor;
        }
    }
    static std::size_t previous_cursor(const text_input_state& state)
    {
        if (state.cursor == 0) return 0;
        std::size_t position = state.cursor - 1;
        while (position > 0 && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u) --position;
        return position;
    }
    static std::size_t next_cursor(const text_input_state& state, std::size_t position)
    {
        if (position >= state.value.size()) return state.value.size();
        ++position;
        while (position < state.value.size() && (static_cast<unsigned char>(state.value[position]) & 0xC0u) == 0x80u) ++position;
        return position;
    }

    teacup::style text_style_;
    teacup::style cursor_style_ = teacup::style{}.reverse();
    renderer_fn renderer_;
};

/// A single-line UTF-8 text editor.
class text_input
{
public:
    text_input() = default;

    text_input& value(std::string value)
    {
        value_ = std::move(value);
        cursor_ = value_.size();
        return *this;
    }
    text_input& cursor(std::size_t position)
    {
        cursor_ = (std::min)(position, value_.size());
        normalize_cursor();
        return *this;
    }
    text_input& text_style(const teacup::style& value)
    {
        text_style_ = value;
        return *this;
    }
    text_input& cursor_style(const teacup::style& value)
    {
        cursor_style_ = value;
        return *this;
    }
    [[nodiscard]] const std::string& value() const
    {
        return value_;
    }

    [[nodiscard]] std::size_t cursor() const
    {
        return cursor_;
    }

    bool update(const key_event& event)
    {
        if (event.code == key::space)
        {
            value_.insert(cursor_, " ");
            ++cursor_;
            return true;
        }

        if (event.code == key::character && !event.text.empty() && !has(event.modifiers, modifier::ctrl)
            && !has(event.modifiers, modifier::alt))
        {
            value_.insert(cursor_, event.text);
            cursor_ += event.text.size();
            normalize_cursor();
            return true;
        }

        switch (event.code)
        {
        case key::left:
            cursor_ = previous_cursor(cursor_);
            return true;
        case key::right:
            cursor_ = next_cursor(cursor_);
            return true;
        case key::home:
            cursor_ = 0;
            return true;
        case key::end:
            cursor_ = value_.size();
            return true;
        case key::backspace:
            if (cursor_ > 0)
            {
                const std::size_t start = previous_cursor(cursor_);
                value_.erase(start, cursor_ - start);
                cursor_ = start;
            }
            return true;
        case key::del:
            if (cursor_ < value_.size())
            {
                value_.erase(cursor_, next_cursor(cursor_) - cursor_);
            }
            return true;
        default:
            return false;
        }
    }

    bool update(const paste_event& event)
    {
        value_.insert(cursor_, event.text);
        cursor_ += event.text.size();
        normalize_cursor();
        return true;
    }

    void render(canvas target) const
    {
        if (!target.valid())
        {
            return;
        }

        const int width = target.width();
        if (width <= 0)
        {
            return;
        }

        target.fill(" ", text_style_);
        const int cursor_width = cursor_ < value_.size()
            ? utf8::width(value_.substr(cursor_, next_cursor(cursor_) - cursor_))
            : 1;
        int start = 0;
        while (utf8::width(value_.substr(start, cursor_ - start)) + cursor_width > width && start < cursor_)
        {
            start = next_cursor(start);
        }

        const std::string visible = value_.substr(start);
        target.text(0, 0, visible, text_style_);
        const int cursor_x = utf8::width(value_.substr(start, cursor_ - start));
        if (cursor_x < width)
        {
            if (cursor_ < value_.size())
            {
                target.text(cursor_x, 0, value_.substr(cursor_, next_cursor(cursor_) - cursor_), cursor_style_);
            }
            else
            {
                target.text(cursor_x, 0, " ", cursor_style_);
            }
        }
    }

private:
    void normalize_cursor()
    {
        while (cursor_ > 0 && cursor_ < value_.size()
               && (static_cast<unsigned char>(value_[cursor_]) & 0xC0u) == 0x80u)
        {
            --cursor_;
        }
    }

    [[nodiscard]] std::size_t previous_cursor(std::size_t position) const
    {
        if (position == 0)
        {
            return 0;
        }
        --position;
        while (position > 0 && (static_cast<unsigned char>(value_[position]) & 0xC0u) == 0x80u)
        {
            --position;
        }
        return position;
    }

    [[nodiscard]] std::size_t next_cursor(std::size_t position) const
    {
        if (position >= value_.size())
        {
            return value_.size();
        }
        ++position;
        while (position < value_.size() && (static_cast<unsigned char>(value_[position]) & 0xC0u) == 0x80u)
        {
            ++position;
        }
        return position;
    }

    std::string value_;
    std::size_t cursor_ = 0;
    teacup::style text_style_;
    teacup::style cursor_style_ = teacup::style{}.reverse();
};

}  // namespace widgets
}  // namespace teacup