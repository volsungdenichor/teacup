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
class mailbox
{
public:
    void push(message msg)
    {
        std::lock_guard<std::mutex> lock{ mutex_ };
        if (closed_)
        {
            return;
        }
        queue_.push_back(std::move(msg));
    }

    bool try_pop(message& out)
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
    std::deque<message> queue_{};
    bool closed_ = false;
};

void spawn(const cmd& command, const std::shared_ptr<mailbox>& box,
           const std::shared_ptr<std::atomic<int>>& pending)
{
    for (const auto& fn : command.items())
    {
        if (!fn)
        {
            continue;
        }
        pending->fetch_add(1);
        std::thread{ [fn, box, pending] {
            message result = fn();
            if (!result.empty())
            {
                box->push(std::move(result));
            }
            pending->fetch_sub(1);
        } }.detach();
    }
}

/// Restores the terminal no matter how the loop is left.
class terminal_guard
{
public:
    terminal_guard(detail::terminal& term, const options& opts)
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
    detail::terminal& term_;
    bool alt_screen_;
    bool mouse_;
    bool paste_;
};

}  // namespace

cmd sequence(std::vector<cmd> cmds)
{
    return cmd{ [cmds = std::move(cmds)]() -> message {
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
        return message{};
    } };
}

namespace detail
{

int run_app(app_base& app, const options& opts)
{
    std::unique_ptr<terminal> term = make_terminal();
    if (!term)
    {
        return 1;
    }

    terminal_guard guard{ *term, opts };
    renderer render{ *term, opts.profile };
    input_decoder decoder;

    auto box = std::make_shared<mailbox>();
    auto pending = std::make_shared<std::atomic<int>>(0);

    surface frame{ term->size() };
    bool dirty = true;
    bool running = true;

    const auto handle = [&](const message& msg) {
        if (msg.empty())
        {
            return;
        }
        if (msg.is<quit_message>())
        {
            running = false;
            return;
        }
        if (const auto* alt = msg_cast<set_alt_screen_message>(msg))
        {
            guard.set_alt_screen(alt->enabled);
            render.invalidate();
            dirty = true;
            return;
        }
        if (const auto* cursor = msg_cast<set_cursor_visible_message>(msg))
        {
            term->set_cursor_visible(cursor->visible);
            return;
        }
        if (const auto* mouse = msg_cast<set_mouse_enabled_message>(msg))
        {
            guard.set_mouse(mouse->enabled);
            return;
        }
        if (const auto* resize = msg_cast<resize_event>(msg))
        {
            frame.resize(resize->size);
            render.invalidate();
        }
        spawn(app.update(msg), box, pending);
        dirty = true;
    };

    handle(message{ start_event{} });
    spawn(app.init(), box, pending);

    const int timeout_ms = static_cast<int>(opts.frame_interval.count());

    while (running)
    {
        bool resized = false;
        const std::string bytes = term->read(timeout_ms, resized);

        if (resized)
        {
            handle(message{ resize_event{ term->size() } });
        }
        if (!bytes.empty())
        {
            for (const message& msg : decoder.feed(bytes))
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
            for (const message& msg : decoder.flush())
            {
                handle(msg);
            }
        }

        message async;
        while (running && box->try_pop(async))
        {
            handle(async);
        }

        if (!running)
        {
            break;
        }

        const teacup::size current = term->size();
        if (current != frame.size())
        {
            handle(message{ resize_event{ current } });
        }

        if (dirty)
        {
            frame.clear();
            app.view(canvas{ frame });
            render.draw(frame);
            dirty = false;
        }
    }

    box->close();
    return 0;
}

}  // namespace detail
}  // namespace teacup
