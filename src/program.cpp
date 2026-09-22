#include "teacup/program.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>

#include "teacup/detail/input_decoder.hpp"
#include "teacup/detail/renderer.hpp"
#include "teacup/detail/terminal.hpp"
#include "teacup/surface.hpp"

namespace teacup
{
namespace
{

/// Thread-safe mailbox shared with command workers.
class Mailbox
{
public:
    void push(Message msg)
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        if (closed_)
        {
            return;
        }
        queue_.push_back(std::move(msg));
    }

    bool try_pop(Message& out)
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        if (queue_.empty())
        {
            return false;
        }
        out = std::move(queue_.front());
        queue_.pop_front();
        return true;
    }

    void close()
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        closed_ = true;
        queue_.clear();
    }

private:
    std::mutex mutex_{};
    std::deque<Message> queue_{};
    bool closed_ = false;
};

void spawn(const Cmd& command, const std::shared_ptr<Mailbox>& box, const std::shared_ptr<std::atomic<int>>& pending)
{
    for (const auto& fn : command.items())
    {
        if (!fn)
        {
            continue;
        }
        pending->fetch_add(1);
        std::thread{
            [fn, box, pending]
            {
                Message result = fn();
                if (!result.empty())
                {
                    box->push(std::move(result));
                }
                pending->fetch_sub(1);
            }
        }.detach();
    }
}

class terminal_guard
{
public:
    terminal_guard(detail::Terminal& term, const Options& opts)
        : term_{ term }
        , alt_screen_{ opts.alt_screen }
        , mouse_{ opts.mouse }
        , paste_{ opts.bracketed_paste }
    {
        term_.enter_raw_mode();
        if (alt_screen_)
        {
            term_.set_alt_screen(true);
        }
        if (opts.hide_cursor)
        {
            term_.set_cursor_visible(false);
        }
        if (mouse_)
        {
            term_.set_mouse_enabled(true);
        }
        if (paste_)
        {
            term_.set_bracketed_paste(true);
        }
        term_.flush();
    }

    ~terminal_guard()
    {
        if (paste_)
        {
            term_.set_bracketed_paste(false);
        }
        if (mouse_)
        {
            term_.set_mouse_enabled(false);
        }
        term_.set_cursor_visible(true);
        term_.write("\x1b[0m");
        if (alt_screen_)
        {
            term_.set_alt_screen(false);
        }
        else
        {
            term_.write("\r\n");
        }
        term_.flush();
        term_.leave_raw_mode();
    }

    terminal_guard(const terminal_guard&) = delete;
    terminal_guard& operator=(const terminal_guard&) = delete;

    void set_alt_screen(bool enabled)
    {
        alt_screen_ = enabled;
        term_.set_alt_screen(enabled);
    }
    void set_mouse(bool enabled)
    {
        mouse_ = enabled;
        term_.set_mouse_enabled(enabled);
    }

private:
    detail::Terminal& term_;
    bool alt_screen_;
    bool mouse_;
    bool paste_;
};

}  // namespace

Cmd sequence(std::vector<Cmd> cmds)
{
    return Cmd{ [cmds = std::move(cmds)]() -> Message
                {
                    for (const auto& c : cmds)
                    {
                        for (const auto& fn : c.items())
                        {
                            if (fn)
                            {
                                fn();
                            }
                        }
                    }
                    return Message{};
                } };
}

namespace detail
{

int run_app(AppBase& app, const Options& opts)
{
    std::unique_ptr<Terminal> term = make_terminal();
    if (!term)
    {
        return 1;
    }

    terminal_guard guard{ *term, opts };
    Renderer render{ *term, opts.profile };
    InputDecoder decoder;

    auto box = std::make_shared<Mailbox>();
    auto pending = std::make_shared<std::atomic<int>>(0);

    Surface frame{ term->size() };
    bool dirty = true;
    bool running = true;

    const auto handle = [&](const Message& msg)
    {
        if (msg.empty())
        {
            return;
        }
        if (msg.is<QuitMessage>())
        {
            running = false;
            return;
        }
        if (const auto* alt = msg_cast<SetAltScreenMessage>(msg))
        {
            guard.set_alt_screen(alt->enabled);
            render.invalidate();
            dirty = true;
            return;
        }
        if (const auto* cursor = msg_cast<SetCursorVisibleMessage>(msg))
        {
            term->set_cursor_visible(cursor->visible);
            return;
        }
        if (const auto* mouse = msg_cast<SetMouseEnabledMessage>(msg))
        {
            guard.set_mouse(mouse->enabled);
            return;
        }
        if (const auto* resize = msg_cast<ResizeEvent>(msg))
        {
            frame.resize(resize->size);
            render.invalidate();
        }
        spawn(app.update(msg), box, pending);
        dirty = true;
    };

    handle(Message{ StartEvent{} });
    spawn(app.init(), box, pending);

    const int timeout_ms = static_cast<int>(opts.frame_interval.count());

    while (running)
    {
        bool resized = false;
        const std::string bytes = term->read(timeout_ms, resized);

        if (resized)
        {
            handle(Message{ ResizeEvent{ term->size() } });
        }
        if (!bytes.empty())
        {
            for (const Message& msg : decoder.feed(bytes))
            {
                handle(msg);
                if (!running)
                {
                    break;
                }
            }
        }
        else
        {
            for (const Message& msg : decoder.flush())
            {
                handle(msg);
            }
        }

        Message async;
        while (running && box->try_pop(async))
        {
            handle(async);
        }

        if (!running)
        {
            break;
        }

        const Size current = term->size();
        if (current != frame.size())
        {
            handle(Message{ ResizeEvent{ current } });
        }

        if (dirty)
        {
            frame.clear();
            app.view(Canvas{ frame });
            render.draw(frame);
            dirty = false;
        }
    }

    box->close();
    return 0;
}

}  // namespace detail
}  // namespace teacup
