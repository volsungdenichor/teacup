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

struct TickMessage
{
};

teacup::Cmd schedule_tick()
{
    return teacup::tick(std::chrono::milliseconds{ 120 }, [] { return TickMessage{}; });
}

teacup::Cmd init(const model&)
{
    return schedule_tick();
}

teacup::UpdateResult<model> update(model m, const teacup::Message& msg)
{
    if (msg.is<TickMessage>())
    {
        m.ticks = (m.ticks + 1) % 100;
        return teacup::next(m, schedule_tick());
    }
    if (const auto* key = teacup::msg_cast<teacup::KeyEvent>(msg))
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

void view(const model& m, teacup::Canvas screen)
{
    using namespace teacup;

    const auto rows = split_vertical(screen.bounds(), { Constraint::fixed(9), Constraint::flex(), Constraint::fixed(1) });

    const Rect card = centered(rows[0], 44, 9);

    widgets::Block{}
        .border(Border::rounded())
        .border_style(Style{}.fg(palette::accent()))
        .title(" teacup ")
        .title_style(Style{}.fg(palette::accent()).bold())
        .title_align(Align::center)
        .padding(2, 1, 2, 1)
        .render(
            screen.sub(card),
            [&](Canvas inner)
            {
                const auto lines = split_vertical(
                    inner.bounds(),
                    { Constraint::fixed(1),
                      Constraint::fixed(1),
                      Constraint::fixed(1),
                      Constraint::fixed(1),
                      Constraint::flex() });

                widgets::Text{ "counter" }
                    .style(Style{}.fg(palette::subtle()))
                    .align(Align::center)
                    .render(inner.sub(lines[0]));

                widgets::Text{ std::to_string(m.count) }
                    .style(Style{}.fg(palette::text()).bold())
                    .align(Align::center)
                    .render(inner.sub(lines[1]));

                widgets::Progress::View{}
                    .gradient(Color::hex("#7d56f4"), Color::hex("#43bf6d"))
                    .render(widgets::Progress::State{ m.ticks / 99.0 }, inner.sub(lines[3]));
            });

    widgets::Text{ "last key: " + m.last_key }
        .style(Style{}.fg(palette::subtle()))
        .align(Align::center)
        .render(screen.sub(rows[1]));

    widgets::Text{ " ↑/↓ count · r reset · q quit " }
        .style(Style{}.fg(palette::surface()).bg(palette::accent()))
        .align(Align::center)
        .render(screen.sub(rows[2]));
}

}  // namespace

int main()
{
    teacup::Options opts;
    opts.alt_screen = true;
    return teacup::run(teacup::make_app(model{}, init, update, view), opts);
}
