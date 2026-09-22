#include "teacup/detail/input_decoder.hpp"

#include <cstdlib>

#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace
{

const char* key_name(Key code)
{
    switch (code)
    {
        case Key::enter: return "enter";
        case Key::tab: return "tab";
        case Key::backspace: return "backspace";
        case Key::escape: return "esc";
        case Key::space: return "space";
        case Key::up: return "up";
        case Key::down: return "down";
        case Key::left: return "left";
        case Key::right: return "right";
        case Key::home: return "home";
        case Key::end: return "end";
        case Key::page_up: return "pgup";
        case Key::page_down: return "pgdown";
        case Key::insert: return "insert";
        case Key::del: return "delete";
        case Key::f1: return "f1";
        case Key::f2: return "f2";
        case Key::f3: return "f3";
        case Key::f4: return "f4";
        case Key::f5: return "f5";
        case Key::f6: return "f6";
        case Key::f7: return "f7";
        case Key::f8: return "f8";
        case Key::f9: return "f9";
        case Key::f10: return "f10";
        case Key::f11: return "f11";
        case Key::f12: return "f12";
        case Key::character:
        case Key::none:
        default: return "";
    }
}

}  // namespace

std::string KeyEvent::name() const
{
    std::string out;
    if (has(modifiers, Modifier::ctrl))
    {
        out += "ctrl+";
    }
    if (has(modifiers, Modifier::alt))
    {
        out += "alt+";
    }
    if (has(modifiers, Modifier::shift))
    {
        out += "shift+";
    }
    out += code == Key::character ? text : key_name(code);
    return out;
}

namespace detail
{
namespace
{

Message make_key(Key code, Modifier mods = Modifier::none)
{
    KeyEvent ev;
    ev.code = code;
    ev.modifiers = mods;
    return Message{ ev };
}

Message make_char(std::string text, Modifier mods = Modifier::none)
{
    KeyEvent ev;
    ev.code = Key::character;
    ev.text = std::move(text);
    ev.modifiers = mods;
    return Message{ ev };
}

Modifier csi_modifiers(int param)
{
    if (param <= 1)
    {
        return Modifier::none;
    }
    const int bits = param - 1;
    Modifier mods = Modifier::none;
    if (bits & 1)
    {
        mods = mods | Modifier::shift;
    }
    if (bits & 2)
    {
        mods = mods | Modifier::alt;
    }
    if (bits & 4)
    {
        mods = mods | Modifier::ctrl;
    }
    return mods;
}

Key key_from_tilde(int code)
{
    switch (code)
    {
        case 1:
        case 7: return Key::home;
        case 2: return Key::insert;
        case 3: return Key::del;
        case 4:
        case 8: return Key::end;
        case 5: return Key::page_up;
        case 6: return Key::page_down;
        case 11: return Key::f1;
        case 12: return Key::f2;
        case 13: return Key::f3;
        case 14: return Key::f4;
        case 15: return Key::f5;
        case 17: return Key::f6;
        case 18: return Key::f7;
        case 19: return Key::f8;
        case 20: return Key::f9;
        case 21: return Key::f10;
        case 23: return Key::f11;
        case 24: return Key::f12;
        default: return Key::none;
    }
}

Key key_from_final(char final_byte)
{
    switch (final_byte)
    {
        case 'A': return Key::up;
        case 'B': return Key::down;
        case 'C': return Key::right;
        case 'D': return Key::left;
        case 'H': return Key::home;
        case 'F': return Key::end;
        case 'P': return Key::f1;
        case 'Q': return Key::f2;
        case 'R': return Key::f3;
        case 'S': return Key::f4;
        default: return Key::none;
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
        const std::string part = params.substr(start, sep == std::string::npos ? std::string::npos : sep - start);
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
bool decode_sgr_mouse(const std::string& params, char final_byte, Message& out)
{
    const std::vector<int> values = split_params(params.substr(1));
    if (values.size() < 3)
    {
        return false;
    }
    MouseEvent ev;
    const int code = values[0];
    ev.position = Point{ values[1] - 1, values[2] - 1 };
    if (code & 4)
    {
        ev.modifiers = ev.modifiers | Modifier::shift;
    }
    if (code & 8)
    {
        ev.modifiers = ev.modifiers | Modifier::alt;
    }
    if (code & 16)
    {
        ev.modifiers = ev.modifiers | Modifier::ctrl;
    }

    if (code & 64)
    {
        ev.button = (code & 1) ? MouseButton::wheel_down : MouseButton::wheel_up;
        ev.action = MouseAction::press;
    }
    else
    {
        switch (code & 3)
        {
            case 0: ev.button = MouseButton::left; break;
            case 1: ev.button = MouseButton::middle; break;
            case 2: ev.button = MouseButton::right; break;
            default: ev.button = MouseButton::none; break;
        }
        if (code & 32)
        {
            ev.action = MouseAction::motion;
        }
        else
        {
            ev.action = final_byte == 'm' ? MouseAction::release : MouseAction::press;
        }
    }
    out = Message{ ev };
    return true;
}

}  // namespace

std::vector<Message> InputDecoder::feed(const std::string& bytes)
{
    buffer_ += bytes;
    std::vector<Message> events;

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
            events.push_back(Message{ PasteEvent{ std::move(paste_) } });
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
                events.push_back(make_key(Key::enter));
                ++pos;
            }
            else if (c == '\t')
            {
                events.push_back(make_key(Key::tab));
                ++pos;
            }
            else if (c == 0x7f || c == 0x08)
            {
                events.push_back(make_key(Key::backspace));
                ++pos;
            }
            else if (c == ' ')
            {
                events.push_back(make_key(Key::space));
                ++pos;
            }
            else if (c == 0)
            {
                events.push_back(make_key(Key::space, Modifier::ctrl));
                ++pos;
            }
            else if (c < 0x20)
            {
                events.push_back(make_char(std::string(1, static_cast<char>('a' + c - 1)), Modifier::ctrl));
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
                   && (static_cast<unsigned char>(buffer_[i]) < 0x40 || static_cast<unsigned char>(buffer_[i]) > 0x7e))
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
                Message mouse;
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
                events.push_back(Message{ FocusEvent{ final_byte == 'I' } });
                pos += consumed;
                continue;
            }

            const std::vector<int> values = split_params(params);
            if (final_byte == '~')
            {
                const Key code = key_from_tilde(values.empty() ? 0 : values[0]);
                if (code != Key::none)
                {
                    events.push_back(make_key(code, csi_modifiers(values.size() > 1 ? values[1] : 1)));
                }
            }
            else
            {
                const Key code = key_from_final(final_byte);
                if (code != Key::none)
                {
                    events.push_back(make_key(code, csi_modifiers(values.size() > 1 ? values[1] : 1)));
                }
            }
            pos += consumed;
            continue;
        }

        std::size_t next = pos + 1;
        const char32_t cp = utf8::decode(buffer_, next);
        if (cp == 0x1b)
        {
            events.push_back(make_key(Key::escape));
        }
        else
        {
            std::string glyph;
            utf8::encode(cp, glyph);
            events.push_back(make_char(std::move(glyph), Modifier::alt));
        }
        pos = next;
    }

    buffer_.erase(0, pos);
    return events;
}

std::vector<Message> InputDecoder::flush()
{
    std::vector<Message> events;
    if (buffer_ == "\x1b")
    {
        events.push_back(make_key(Key::escape));
        buffer_.clear();
    }
    return events;
}

}  // namespace detail
}  // namespace teacup
