#include "teacup/detail/terminal.hpp"

#if defined(_WIN32)

namespace teacup
{
namespace detail
{
std::unique_ptr<Terminal> make_terminal()
{
    return nullptr;  // Windows VT backend: see docs/PLAN.md milestone M7.
}
}  // namespace detail
}  // namespace teacup

#else

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <vector>

namespace teacup
{
namespace detail
{
namespace
{

std::atomic<bool> g_resized{ false };

void on_sigwinch(int)
{
    g_resized.store(true, std::memory_order_relaxed);
}

class PosixTerminal final : public Terminal
{
public:
    PosixTerminal() : in_fd_{ ::isatty(STDIN_FILENO) ? STDIN_FILENO : -1 }, out_fd_{ STDOUT_FILENO } { }

    ~PosixTerminal() override { leave_raw_mode(); }

    bool is_tty() const override { return in_fd_ >= 0 && ::isatty(out_fd_) != 0; }

    Size size() const override
    {
        winsize ws{};
        if (::ioctl(out_fd_, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0)
        {
            return { static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row) };
        }
        return { 80, 24 };
    }

    void enter_raw_mode() override
    {
        if (raw_ || in_fd_ < 0)
        {
            return;
        }
        if (::tcgetattr(in_fd_, &saved_) != 0)
        {
            return;
        }
        termios raw = saved_;
        raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL | BRKINT | INPCK | ISTRIP));
        raw.c_oflag &= static_cast<tcflag_t>(~OPOST);
        raw.c_lflag &= static_cast<tcflag_t>(~(ECHO | ICANON | IEXTEN | ISIG));
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 0;
        if (::tcsetattr(in_fd_, TCSAFLUSH, &raw) != 0)
        {
            return;
        }
        raw_ = true;

        struct sigaction sa
        {
        };
        sa.sa_handler = &on_sigwinch;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        ::sigaction(SIGWINCH, &sa, &saved_winch_);
    }

    void leave_raw_mode() override
    {
        if (!raw_)
        {
            return;
        }
        ::sigaction(SIGWINCH, &saved_winch_, nullptr);
        ::tcsetattr(in_fd_, TCSAFLUSH, &saved_);
        raw_ = false;
    }

    void write(const std::string& data) override { buffer_ += data; }

    void flush() override
    {
        const char* data = buffer_.data();
        std::size_t left = buffer_.size();
        while (left > 0)
        {
            const ssize_t written = ::write(out_fd_, data, left);
            if (written <= 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                break;
            }
            data += written;
            left -= static_cast<std::size_t>(written);
        }
        buffer_.clear();
    }

    std::pair<std::string, bool> read(int timeout_ms) override
    {
        bool resized = g_resized.exchange(false, std::memory_order_relaxed);
        if (in_fd_ < 0)
        {
            return { std::string{}, resized };
        }

        fd_set set;
        FD_ZERO(&set);
        FD_SET(in_fd_, &set);
        timeval tv{};
        std::tie(tv.tv_sec, tv.tv_usec) = std::make_pair(timeout_ms / 1000, (timeout_ms % 1000) * 1000);

        const int ready = ::select(in_fd_ + 1, &set, nullptr, nullptr, &tv);
        if (ready <= 0)
        {
            resized = resized || g_resized.exchange(false, std::memory_order_relaxed);
            return { std::string{}, resized };
        }

        char chunk[1024];
        const ssize_t count = ::read(in_fd_, chunk, sizeof(chunk));
        if (count <= 0)
        {
            return { std::string{}, resized };
        }
        return { std::string{ chunk, static_cast<std::size_t>(count) }, resized };
    }

    void set_alt_screen(bool enabled) override { write(enabled ? "\x1b[?1049h" : "\x1b[?1049l"); }

    void set_cursor_visible(bool visible) override { write(visible ? "\x1b[?25h" : "\x1b[?25l"); }

    void set_mouse_enabled(bool enabled) override
    {
        write(enabled ? "\x1b[?1000h\x1b[?1002h\x1b[?1006h" : "\x1b[?1006l\x1b[?1002l\x1b[?1000l");
    }

    void set_bracketed_paste(bool enabled) override { write(enabled ? "\x1b[?2004h" : "\x1b[?2004l"); }

private:
    int in_fd_;
    int out_fd_;
    bool raw_ = false;
    termios saved_{};
    struct sigaction saved_winch_
    {
    };
    std::string buffer_{};
};

}  // namespace

std::unique_ptr<Terminal> make_terminal()
{
    return std::make_unique<PosixTerminal>();
}

}  // namespace detail
}  // namespace teacup

#endif
