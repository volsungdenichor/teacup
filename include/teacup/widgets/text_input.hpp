#pragma once

#include <algorithm>
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

/// A single-line UTF-8 text editor.
class text_input
{
public:
    text_input() = default;

    text_input& value(std::string value) &
    {
        value_ = std::move(value);
        cursor_ = value_.size();
        return *this;
    }
    text_input&& value(std::string value) &&
    {
        return std::move(this->value(std::move(value)));
    }

    text_input& cursor(std::size_t position) &
    {
        cursor_ = (std::min)(position, value_.size());
        normalize_cursor();
        return *this;
    }
    text_input&& cursor(std::size_t position) &&
    {
        return std::move(this->cursor(position));
    }

    text_input& text_style(const teacup::style& value) &
    {
        text_style_ = value;
        return *this;
    }
    text_input&& text_style(const teacup::style& value) &&
    {
        return std::move(this->text_style(value));
    }

    text_input& cursor_style(const teacup::style& value) &
    {
        cursor_style_ = value;
        return *this;
    }
    text_input&& cursor_style(const teacup::style& value) &&
    {
        return std::move(this->cursor_style(value));
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