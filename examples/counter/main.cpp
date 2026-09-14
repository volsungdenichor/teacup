#include <chrono>
#include <string>

#include "teacup/teacup.hpp"

namespace
{

struct model
{
    int count = 0;
    int ticks = 0;
    std::string last_key = "-";
};

struct tick_message
{
};

teacup::cmd schedule_tick()
{
    return teacup::tick(std::chrono::milliseconds{ 120 }, [] { return tick_message{}; });
}

teacup::cmd init(const model&)
{
    return schedule_tick();
}

teacup::update_result<model> update(model m, const teacup::message& msg)
{
    if (msg.is<tick_message>())
    {
        m.ticks = (m.ticks + 1) % 100;
        return teacup::next(m, schedule_tick());
    }
    if (const auto* key = teacup::msg_cast<teacup::key_event>(msg))
    {
        m.last_key = key->name();
        if (key->matches("ctrl+c") || key->matches("q") || key->matches("esc"))
        {
            return teacup::next(m, teacup::quit());
        }
        if (key->matches("up") || key->matches("k"))
        {
            ++m.count;
        }
        if (key->matches("down") || key->matches("j"))
        {
            --m.count;
        }
        if (key->matches("r"))
        {
            m.count = 0;
        }
    }
    return teacup::next(m);
}

void view(const model& m, teacup::canvas screen)
{
    using namespace teacup;

    const auto rows = split_vertical(
        screen.bounds(), { constraint::fixed(9), constraint::flex(), constraint::fixed(1) });

    const rect card = centered(rows[0], 44, 9);

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::accent()))
        .title(" teacup ")
        .title_style(style{}.fg(palette::accent()).bold())
        .title_align(align::center)
        .padding(2, 1, 2, 1)
        .render(screen.sub(card), [&](canvas inner) {
            const auto lines = split_vertical(
                inner.bounds(),
                { constraint::fixed(1),
                  constraint::fixed(1),
                  constraint::fixed(1),
                  constraint::fixed(1),
                  constraint::flex() });

            widgets::text{ "counter" }
                .style(style{}.fg(palette::subtle()))
                .align(align::center)
                .render(inner.sub(lines[0]));

            widgets::text{ std::to_string(m.count) }
                .style(style{}.fg(palette::text()).bold())
                .align(align::center)
                .render(inner.sub(lines[1]));

            widgets::progress{ m.ticks / 99.0 }
                .gradient(color::hex("#7d56f4"), color::hex("#43bf6d"))
                .render(inner.sub(lines[3]));
        });

    widgets::text{ "last key: " + m.last_key }
        .style(style{}.fg(palette::subtle()))
        .align(align::center)
        .render(screen.sub(rows[1]));

    widgets::text{ " ↑/↓ count · r reset · q quit " }
        .style(style{}.fg(palette::surface()).bg(palette::accent()))
        .align(align::center)
        .render(screen.sub(rows[2]));
}

}  // namespace

int main()
{
    teacup::options opts;
    opts.alt_screen = true;
    return teacup::run(teacup::make_app(model{}, init, update, view), opts);
}
