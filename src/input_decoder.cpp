#include "teacup/detail/input_decoder.hpp"

#include <cstdlib>

#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace
{

const char* key_name(key code)
{
    switch (code)
    {
        case key::enter: return "enter";
        case key::tab: return "tab";
        case key::backspace: return "backspace";
        case key::escape: return "esc";
        case key::space: return "space";
        case key::up: return "up";
        case key::down: return "down";
        case key::left: return "left";
        case key::right: return "right";
        case key::home: return "home";
        case key::end: return "end";
        case key::page_up: return "pgup";
        case key::page_down: return "pgdown";
        case key::insert: return "insert";
        case key::del: return "delete";
        case key::f1: return "f1";
        case key::f2: return "f2";
        case key::f3: return "f3";
        case key::f4: return "f4";
        case key::f5: return "f5";
        case key::f6: return "f6";
        case key::f7: return "f7";
        case key::f8: return "f8";
        case key::f9: return "f9";
        case key::f10: return "f10";
        case key::f11: return "f11";
        case key::f12: return "f12";
        case key::character:
        case key::none:
        default: return "";
    }
}

}  // namespace

std::string key_event::name() const
{
    std::string out;
    if (has(modifiers, modifier::ctrl))
    {
        out += "ctrl+";
    }
    if (has(modifiers, modifier::alt))
    {
        out += "alt+";
    }
    if (has(modifiers, modifier::shift))
    {
        out += "shift+";
    }
    out += code == key::character ? text : key_name(code);
    return out;
}

namespace detail
{
namespace
{

message make_key(key code, modifier mods = modifier::none)
{
    key_event ev;
    ev.code = code;
    ev.modifiers = mods;
    return message{ ev };
}

message make_char(std::string text, modifier mods = modifier::none)
{
    key_event ev;
    ev.code = key::character;
    ev.text = std::move(text);
    ev.modifiers = mods;
    return message{ ev };
}

modifier csi_modifiers(int param)
{
    if (param <= 1)
    {
        return modifier::none;
    }
    const int bits = param - 1;
    modifier mods = modifier::none;
    if (bits & 1)
    {
        mods = mods | modifier::shift;
    }
    if (bits & 2)
    {
        mods = mods | modifier::alt;
    }
    if (bits & 4)
    {
        mods = mods | modifier::ctrl;
    }
    return mods;
}

key key_from_tilde(int code)
{
    switch (code)
    {
        case 1:
        case 7: return key::home;
        case 2: return key::insert;
        case 3: return key::del;
        case 4:
        case 8: return key::end;
        case 5: return key::page_up;
        case 6: return key::page_down;
        case 11: return key::f1;
        case 12: return key::f2;
        case 13: return key::f3;
        case 14: return key::f4;
        case 15: return key::f5;
        case 17: return key::f6;
        case 18: return key::f7;
        case 19: return key::f8;
        case 20: return key::f9;
        case 21: return key::f10;
        case 23: return key::f11;
        case 24: return key::f12;
        default: return key::none;
    }
}

key key_from_final(char final_byte)
{
    switch (final_byte)
    {
        case 'A': return key::up;
        case 'B': return key::down;
        case 'C': return key::right;
        case 'D': return key::left;
        case 'H': return key::home;
        case 'F': return key::end;
        case 'P': return key::f1;
        case 'Q': return key::f2;
        case 'R': return key::f3;
        case 'S': return key::f4;
        default: return key::none;
    }
}

std::size_t utf8_sequence_length(unsigned char lead)
{
    if ((lead & 0xE0) == 0xC0)
    {
        return 2;
    }
    if ((lead & 0xF0) == 0xE0)
    {
        return 3;
    }
    if ((lead & 0xF8) == 0xF0)
    {
        return 4;
    }
    return 1;
}

std::vector<int> split_params(const std::string& params)
{
    std::vector<int> result;
    std::size_t start = 0;
    while (start <= params.size())
    {
        const std::size_t sep = params.find(';', start);
        const std::string part = params.substr(
            start, sep == std::string::npos ? std::string::npos : sep - start);
        result.push_back(part.empty() ? 0 : std::atoi(part.c_str()));
        if (sep == std::string::npos)
        {
            break;
        }
        start = sep + 1;
    }
    return result;
}

/// Decodes an SGR mouse report body ("<btn;col;row" + 'M'/'m').
bool decode_sgr_mouse(const std::string& params, char final_byte, message& out)
{
    const std::vector<int> values = split_params(params.substr(1));
    if (values.size() < 3)
    {
        return false;
    }
    mouse_event ev;
    const int code = values[0];
    ev.position = point{ values[1] - 1, values[2] - 1 };
    if (code & 4)
    {
        ev.modifiers = ev.modifiers | modifier::shift;
    }
    if (code & 8)
    {
        ev.modifiers = ev.modifiers | modifier::alt;
    }
    if (code & 16)
    {
        ev.modifiers = ev.modifiers | modifier::ctrl;
    }

    if (code & 64)
    {
        ev.button = (code & 1) ? mouse_button::wheel_down : mouse_button::wheel_up;
        ev.action = mouse_action::press;
    }
    else
    {
        switch (code & 3)
        {
            case 0: ev.button = mouse_button::left; break;
            case 1: ev.button = mouse_button::middle; break;
            case 2: ev.button = mouse_button::right; break;
            default: ev.button = mouse_button::none; break;
        }
        if (code & 32)
        {
            ev.action = mouse_action::motion;
        }
        else
        {
            ev.action = final_byte == 'm' ? mouse_action::release : mouse_action::press;
        }
    }
    out = message{ ev };
    return true;
}

}  // namespace

std::vector<message> input_decoder::feed(const std::string& bytes)
{
    buffer_ += bytes;
    std::vector<message> events;

    std::size_t pos = 0;
    while (pos < buffer_.size())
    {
        if (in_paste_)
        {
            const std::size_t end = buffer_.find("\x1b[201~", pos);
            if (end == std::string::npos)
            {
                paste_ += buffer_.substr(pos);
                pos = buffer_.size();
                break;
            }
            paste_ += buffer_.substr(pos, end - pos);
            events.push_back(message{ paste_event{ std::move(paste_) } });
            paste_.clear();
            in_paste_ = false;
            pos = end + 6;
            continue;
        }

        const unsigned char c = static_cast<unsigned char>(buffer_[pos]);

        if (c != 0x1b)
        {
            if (c == '\r' || c == '\n')
            {
                events.push_back(make_key(key::enter));
                ++pos;
            }
            else if (c == '\t')
            {
                events.push_back(make_key(key::tab));
                ++pos;
            }
            else if (c == 0x7f || c == 0x08)
            {
                events.push_back(make_key(key::backspace));
                ++pos;
            }
            else if (c == ' ')
            {
                events.push_back(make_key(key::space));
                ++pos;
            }
            else if (c == 0)
            {
                events.push_back(make_key(key::space, modifier::ctrl));
                ++pos;
            }
            else if (c < 0x20)
            {
                events.push_back(
                    make_char(std::string(1, static_cast<char>('a' + c - 1)), modifier::ctrl));
                ++pos;
            }
            else
            {
                if (pos + utf8_sequence_length(c) > buffer_.size())
                {
                    break;  // incomplete UTF-8 sequence, wait for more bytes
                }
                std::size_t next = pos;
                const char32_t cp = utf8::decode(buffer_, next);
                std::string glyph;
                utf8::encode(cp, glyph);
                events.push_back(make_char(std::move(glyph)));
                pos = next;
            }
            continue;
        }

        // ESC ...
        if (pos + 1 >= buffer_.size())
        {
            break;  // wait for more input; `flush()` resolves a lone ESC
        }

        const char second = buffer_[pos + 1];
        if (second == '[' || second == 'O')
        {
            std::size_t i = pos + 2;
            std::string params;
            while (i < buffer_.size()
                   && (static_cast<unsigned char>(buffer_[i]) < 0x40
                       || static_cast<unsigned char>(buffer_[i]) > 0x7e))
            {
                params += buffer_[i];
                ++i;
            }
            if (i >= buffer_.size())
            {
                break;  // incomplete escape sequence
            }
            const char final_byte = buffer_[i];
            const std::size_t consumed = i + 1 - pos;

            if (!params.empty() && params[0] == '<')
            {
                message mouse;
                if (decode_sgr_mouse(params, final_byte, mouse))
                {
                    events.push_back(std::move(mouse));
                }
                pos += consumed;
                continue;
            }
            if (params == "200" && final_byte == '~')
            {
                in_paste_ = true;
                pos += consumed;
                continue;
            }
            if (final_byte == 'I' || final_byte == 'O')
            {
                events.push_back(message{ focus_event{ final_byte == 'I' } });
                pos += consumed;
                continue;
            }

            const std::vector<int> values = split_params(params);
            if (final_byte == '~')
            {
                const key code = key_from_tilde(values.empty() ? 0 : values[0]);
                if (code != key::none)
                {
                    events.push_back(
                        make_key(code, csi_modifiers(values.size() > 1 ? values[1] : 1)));
                }
            }
            else
            {
                const key code = key_from_final(final_byte);
                if (code != key::none)
                {
                    events.push_back(
                        make_key(code, csi_modifiers(values.size() > 1 ? values[1] : 1)));
                }
            }
            pos += consumed;
            continue;
        }

        // ESC followed by a regular key => alt modifier.
        std::size_t next = pos + 1;
        const char32_t cp = utf8::decode(buffer_, next);
        if (cp == 0x1b)
        {
            events.push_back(make_key(key::escape));
        }
        else
        {
            std::string glyph;
            utf8::encode(cp, glyph);
            events.push_back(make_char(std::move(glyph), modifier::alt));
        }
        pos = next;
    }

    buffer_.erase(0, pos);
    return events;
}

std::vector<message> input_decoder::flush()
{
    std::vector<message> events;
    if (buffer_ == "\x1b")
    {
        events.push_back(make_key(key::escape));
        buffer_.clear();
    }
    return events;
}

}  // namespace detail
}  // namespace teacup
