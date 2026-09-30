#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "teacup/teacup.hpp"

namespace
{

struct TickMessage
{
};

enum class Country
{
    usa,
    czech_republic
};

struct Player
{
    std::string name;
    Country country;
    bool living = false;
};

std::string country_name(Country country)
{
    return country == Country::usa ? "USA" : "Czech Republic";
};

struct Model
{
    int tick = 0;
    teacup::Size screen_size{ 80, 24 };
    teacup::Router events;
    std::string last_key = "-";
    std::string last_mouse = "-";
    teacup::widgets::List<std::string>::State menu;
    teacup::widgets::Tabs::State tabs;
    teacup::widgets::TextInput::State input;
    teacup::widgets::Viewport::State viewport;
    teacup::widgets::Spinner::State spinner;
    teacup::widgets::List<Player>::State players;
};

teacup::layout::Node showcase_tree()
{
    using namespace teacup::layout;
    return column(
        {
            fixed(
                7,
                named(
                    "header",
                    inset(
                        2,
                        column(
                            {
                                fixed(
                                    1,
                                    row({
                                        flex(leaf("title")),
                                        fixed(14, leaf("spinner")),
                                    })),
                                flex(space()),
                                fixed(1, focusable("tabs")),
                            })))),
            flex(row(
                {
                    percent(
                        42,
                        named(
                            "navigation",
                            inset(
                                2,
                                column(
                                    {
                                        fixed(6, focusable("menu")),
                                        flex(focusable("players")),
                                    },
                                    1)))),
                    flex(named(
                        "components",
                        inset(
                            2,
                            column(
                                {
                                    fixed(1, leaf("progress")),
                                    fixed(4, leaf("table")),
                                    fixed(5, named("input-box", inset(1, focusable("input")))),
                                },
                                1)))),
                },
                1)),
            fixed(1, leaf("footer")),
        });
}

void render_player(const Player& player, teacup::Canvas row, const teacup::Style&, bool selected)
{
    using namespace teacup;
    const int width = row.size().width;
    widgets::Text{ player.name }
        .style(Style{}.fg(selected ? palette::accent() : palette::text()).bold())
        .render(row.sub(Rect{ 0, 0, width, 1 }));
    widgets::Text{ "Country: " + country_name(player.country) }
        .style(Style{}.fg(palette::subtle()))
        .render(row.sub(Rect{ 0, 1, width, 1 }));

    widgets::Text{ "Living:" }.style(Style{}.fg(palette::subtle())).render(row.sub(Rect{ 0, 2, 8, 1 }));
    widgets::Checkbox::View{}
        .checked_style(Style{}.fg(palette::success()).bold())
        .unchecked_style(Style{}.fg(palette::danger()))
        .render(widgets::Checkbox::State{ player.living }, row.sub(Rect{ 8, 2, width - 8, 1 }));
}

teacup::Cmd tick()
{
    return teacup::tick(std::chrono::milliseconds{ 180 }, [] { return TickMessage{}; });
}

teacup::Cmd init(const Model&)
{
    return tick();
}

teacup::UpdateResult<Model> update(Model m, const teacup::Message& message)
{
    const teacup::widgets::List<std::string>::View menu_view;
    teacup::widgets::Tabs::View tabs_view;
    const teacup::widgets::TextInput::View input_view;
    const teacup::widgets::Viewport::View viewport_view;
    teacup::widgets::List<Player>::View players_view;
    const teacup::layout::Areas areas = teacup::layout::resolve(showcase_tree(), teacup::Rect::from_size(m.screen_size));
    players_view.item_height(3).viewport_height(areas.get("players").height());

    const auto sync_menu_to_focus = [&]
    {
        const std::string& f = m.events.focus().focused();
        m.menu.selected = f == "input" ? 1 : f == "players" ? 2 : 0;
    };
    const auto activate_menu = [&]
    {
        m.events.focus().set(m.menu.selected == 1 ? "input" : m.menu.selected == 2 ? "players" : "menu");
    };

    m.events.clear();
    m.events.add("menu", teacup::route_keys(m.menu, menu_view));
    m.events.add("tabs", teacup::route_keys(m.tabs, tabs_view));
    m.events.add("tabs", teacup::route_mouse(m.tabs, tabs_view.labels({ "gallery", "layout", "events" }), "tabs"));
    m.events.add("players", teacup::route_keys(m.players, players_view));
    m.events.add("players", teacup::route_mouse(m.players, players_view, "players"));
    m.events.add("input", teacup::route_keys(m.input, input_view));
    m.events.add("input", teacup::route_mouse(m.input, input_view, "input"));

    if (m.events.focus().order().empty())
    {
        m.events.focus_order(areas.focus_order());
        m.events.focus().set("menu");
    }

    if (const auto* mouse = teacup::msg_cast<teacup::MouseEvent>(message))
    {
        m.last_mouse = std::to_string(mouse->position.x) + "," + std::to_string(mouse->position.y);
        m.events.route(message, areas);
        if (menu_view.update(m.menu, *mouse, areas.get("menu")))
        {
            activate_menu();  // clicking a menu row jumps focus to it
        }
        viewport_view.update(m.viewport, *mouse, areas.get("players"));
        if (mouse->action == teacup::MouseAction::press && mouse->button == teacup::MouseButton::left)
        {
            if (const std::string* target = areas.hit(mouse->position); target != nullptr && *target == "players")
            {
                const int row = m.players.offset * 3 + mouse->position.y - areas.get("players").left();
                const int item = row / 3;
                if (row % 3 == 2 && item >= 0 && item < static_cast<int>(m.players.items.size()))
                {
                    m.players.items[static_cast<std::size_t>(item)].living
                        = !m.players.items[static_cast<std::size_t>(item)].living;
                }
            }
        }
        return teacup::next(m);
    }

    if (const auto* resize = teacup::msg_cast<teacup::ResizeEvent>(message))
    {
        m.screen_size = resize->size;
        return teacup::next(m);
    }

    if (message.is<TickMessage>())
    {
        ++m.tick;
        ++m.spinner.frame;
        return teacup::next(m, tick());
    }

    if (const auto* paste = teacup::msg_cast<teacup::PasteEvent>(message))
    {
        if (m.events.focus().is("input"))
        {
            input_view.update(m.input, *paste);
        }

        return teacup::next(m);
    }

    if (const auto* key = teacup::msg_cast<teacup::KeyEvent>(message))
    {
        m.last_key = key->name();
        if (key->matches("ctrl+c"))
        {
            return teacup::next(m, teacup::quit());
        }

        const std::string was_focused = m.events.focus().focused();
        const bool consumed = m.events.route(message, areas);

        if (key->code == teacup::Key::enter)
        {
            if (was_focused == "menu")
            {
                activate_menu();
            }
            else
            {
                m.menu.selected = was_focused == "input" ? 1 : was_focused == "players" ? 2 : 0;
                m.events.focus().set("menu");
            }
            return teacup::next(m);
        }

        if (key->code == teacup::Key::page_up)
        {
            if (was_focused == "players")
            {
                players_view.page_up(m.players, areas.get("players").height());
            }
            else
            {
                viewport_view.page_up(m.viewport, 4);
            }
            return teacup::next(m);
        }
        if (key->code == teacup::Key::page_down)
        {
            if (was_focused == "players")
                players_view.page_down(m.players, areas.get("players").height());
            else
                viewport_view.page_down(m.viewport, 4);
            return teacup::next(m);
        }

        if (!consumed && m.events.focus().is("menu") && m.menu.selected == 1
            && (key->code == teacup::Key::character || key->code == teacup::Key::space || key->code == teacup::Key::backspace
                || key->code == teacup::Key::del || key->code == teacup::Key::home || key->code == teacup::Key::end))
        {
            m.events.focus().set("input");
            sync_menu_to_focus();
            input_view.update(m.input, *key);
            return teacup::next(m);
        }
    }
    return teacup::next(m);
}

void view(const Model& m, teacup::Canvas screen)
{
    using namespace teacup;

    screen.fill(" ", Style{}.bg(palette::surface()));
    const layout::Areas areas = layout::resolve(showcase_tree(), screen.bounds());

    widgets::Block{}
        .border(Border::rounded())
        .border_style(Style{}.fg(palette::accent()))
        .title(" teacup widget gallery ")
        .title_style(Style{}.fg(palette::accent()).bold())
        .title_align(Align::center)
        .padding(1)
        .render(
            screen.sub(areas.get("header")),
            [&](Canvas)
            {
                widgets::Text{ "A small, living tour of the teacup toolkit" }
                    .style(Style{}.fg(palette::text()).bold())
                    .render(screen.sub(areas.get("title")));
                widgets::Spinner::View{}
                    .frames(
                        { "⠋ live",
                          "⠙ live",
                          "⠹ live",
                          "⠸ live",
                          "⠼ live",
                          "⠴ live",
                          "⠦ live",
                          "⠧ live",
                          "⠇ live",
                          "⠏ live" })
                    .style(Style{}.fg(palette::accent()))
                    .render(m.spinner, screen.sub(areas.get("spinner")));
                widgets::Tabs::View{}
                    .labels({ "gallery", "layout", "events" })
                    .tab_style(Style{}.fg(palette::subtle()))
                    .selected_style(Style{}.fg(palette::text()).bg(palette::accent()).bold())
                    .render(m.tabs, screen.sub(areas.get("tabs")));
            });

    widgets::Block{}
        .border(Border::rounded())
        .border_style(Style{}.fg(palette::subtle()))
        .title(" navigation · players offset: " + std::to_string(m.players.offset) + " ")
        .title_style(Style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(areas.get("navigation")),
            [&](Canvas)
            {
                widgets::List<std::string>::View{}
                    .selected_style(
                        !m.events.focus().is("menu") ? Style{}.fg(palette::accent()).bold()
                                                     : Style{}.fg(palette::surface()).bg(palette::accent()).bold())
                    .item_style(Style{}.fg(palette::text()))
                    .render(m.menu, screen.sub(areas.get("menu")));

                widgets::List<Player>::View{}
                    .item_height(3)
                    .selected_style(Style{}.bg(Color::hex("#313244")))
                    .item_style(Style{}.fg(palette::text()))
                    .item_renderer(render_player)
                    .render(m.players, screen.sub(areas.get("players")));
            });

    widgets::Block{}
        .border(Border::rounded())
        .border_style(Style{}.fg(palette::subtle()))
        .title(" components ")
        .title_style(Style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(areas.get("components")),
            [&](Canvas)
            {
                widgets::Progress::View{}
                    .gradient(palette::accent(), Color::hex("#43bf6d"))
                    .render(widgets::Progress::State{ (m.tick % 100) / 99.0 }, screen.sub(areas.get("progress")));

                widgets::Table{}
                    .headers({ "widget", "state" })
                    .rows({ { "list", "ready" }, { "viewport", "scrolling" }, { "tabs", "selectable" } })
                    .header_style(Style{}.fg(palette::accent()).bold())
                    .row_style(Style{}.fg(palette::text()))
                    .render(screen.sub(areas.get("table")));

                widgets::Block{}
                    .border(Border::plain())
                    .border_style(Style{}.fg(palette::subtle()))
                    .title(m.events.focus().is("input") ? " edit me [active] " : " edit me ")
                    .title_style(Style{}.fg(m.events.focus().is("input") ? palette::accent() : palette::subtle()))
                    .padding(0)
                    .render(
                        screen.sub(areas.get("input-box")),
                        [&](Canvas)
                        {
                            const Canvas input_area = screen.sub(areas.get("input"));
                            const int width = input_area.size().width;
                            widgets::TextInput::View{}
                                .text_style(Style{}.fg(palette::text()))
                                .cursor_style(
                                    m.events.focus().is("input") ? Style{}.fg(palette::surface()).bg(palette::accent())
                                                                 : Style{}.fg(palette::subtle()).bg(palette::surface()))
                                .render(m.input, input_area.sub(Rect{ 0, 0, width, 1 }));
                            widgets::Text{ "> " + m.input.value }
                                .style(Style{}.fg(palette::subtle()))
                                .render(input_area.sub(Rect{ 0, 1, width, 1 }));
                            widgets::Text{ "mouse: " + m.last_mouse }
                                .style(Style{}.fg(palette::subtle()))
                                .render(input_area.sub(Rect{ 0, 2, width, 1 }));
                        });
            });

    widgets::Text{ "arrows select · enter activate · tab/shift+tab focus · ctrl+c quit" }
        .style(Style{}.fg(palette::surface()).bg(palette::accent()))
        .align(Align::center)
        .render(screen.sub(areas.get("footer")));
}

}  // namespace

int main()
{
    Model initial;
    initial.menu.items = { "overview", "text input", "players", "table", "progress" };
    initial.tabs.selected = 0;
    initial.input.value = "type here";
    initial.input.cursor = initial.input.value.size();
    initial.viewport.lines = { "viewport", "scrollable", "content", "lives", "inside", "a", "small", "window" };
    initial.players.items = {
        { "Name: McEnroe", Country::usa, true },
        { "Name: Agassi", Country::usa, true },
        { "Name: Navratilova", Country::czech_republic, true },
        { "Name: Mrtvy", Country::czech_republic, false },
        { "Name: Deadmeat", Country::usa, false },
    };

    teacup::Options options;
    options.alt_screen = true;
    options.mouse = true;
    return teacup::run(teacup::make_app(initial, init, update, view), options);
}