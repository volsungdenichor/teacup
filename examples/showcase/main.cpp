#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "teacup/teacup.hpp"

namespace
{

struct tick_message
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

struct model
{
    int tick = 0;
    teacup::size screen_size{ 80, 24 };
    teacup::router events;  // owns the focus ring; survives model copies
    std::string last_key = "-";
    std::string last_mouse = "-";
    teacup::widgets::list<std::string>::state_type menu;
    teacup::widgets::tabs::state_type tabs;
    teacup::widgets::text_input::state_type input;
    teacup::widgets::viewport::state_type viewport;
    teacup::widgets::spinner::state_type spinner;
    teacup::widgets::list<Player>::state_type players;
};

/// The single source of truth for widget placement, shared by `update`
/// (hit-testing) and `view` (painting) so the two can never drift apart.
/// The inset nodes mirror each block's border + padding.
teacup::layout::node showcase_tree()
{
    using namespace teacup::layout;
    return column({
        fixed(7, named("header", inset(2, column({
            fixed(1, row({
                flex(leaf("title")),
                fixed(14, leaf("spinner")),
            })),
            flex(space()),
            fixed(1, focusable("tabs")),
        })))),
        flex(row({
            percent(42, named("navigation", inset(2, column({
                fixed(6, focusable("menu")),
                flex(focusable("players")),
            }, 1)))),
            flex(named("components", inset(2, column({
                fixed(1, leaf("progress")),
                fixed(4, leaf("table")),
                fixed(5, named("input-box", inset(1, focusable("input")))),
            }, 1)))),
        }, 1)),
        fixed(1, leaf("footer")),
    });
}

void render_player(const Player& player, teacup::canvas row, const teacup::style&, bool selected)
{
    using namespace teacup;
    widgets::text{ player.name }
        .style(style{}.fg(selected ? palette::accent() : palette::text()).bold())
        .render(row.sub(rect{ 0, 0, row.width(), 1 }));
    widgets::text{ "Country: " + country_name(player.country) }
        .style(style{}.fg(palette::subtle()))
        .render(row.sub(rect{ 0, 1, row.width(), 1 }));

    widgets::text{ "Living:" }.style(style{}.fg(palette::subtle())).render(row.sub(rect{ 0, 2, 8, 1 }));
    widgets::checkbox::view_type{}
        .checked_style(style{}.fg(palette::success()).bold())
        .unchecked_style(style{}.fg(palette::danger()))
        .render(widgets::checkbox::state_type{ player.living }, row.sub(rect{ 8, 2, row.width() - 8, 1 }));
}

teacup::cmd tick()
{
    return teacup::tick(std::chrono::milliseconds{ 180 }, [] { return tick_message{}; });
}

teacup::cmd init(const model&)
{
    return tick();
}

teacup::update_result<model> update(model m, const teacup::message& message)
{
    const teacup::widgets::list<std::string>::view_type menu_view;
    teacup::widgets::tabs::view_type tabs_view;
    const teacup::widgets::text_input::view_type input_view;
    const teacup::widgets::viewport::view_type viewport_view;
    teacup::widgets::list<Player>::view_type players_view;
    const teacup::layout::areas areas
        = teacup::layout::resolve(showcase_tree(), teacup::rect::from_size(m.screen_size));
    players_view.item_height(3).viewport_height(areas.get("players").height);

    // Focus is authoritative; the menu selection mirrors it so the hub
    // highlights the active widget. Enter on the hub activates (focuses) the
    // selected widget; enter on a widget backs out to the hub.
    const auto sync_menu_to_focus = [&]
    {
        const std::string& f = m.events.focus().focused();
        m.menu.selected = f == "input" ? 1 : f == "players" ? 2 : 0;
    };
    const auto activate_menu = [&]
    {
        m.events.focus().set(m.menu.selected == 1 ? "input" : m.menu.selected == 2 ? "players" : "menu");
    };

    m.events.clear();  // rebind against this frame's model copy
    m.events.add("menu", teacup::route_keys(m.menu, menu_view));
    m.events.add("tabs", teacup::route_keys(m.tabs, tabs_view));
    m.events.add("tabs", teacup::route_mouse(m.tabs, tabs_view.labels({ "gallery", "layout", "events" }), "tabs"));
    m.events.add("players", teacup::route_keys(m.players, players_view));
    m.events.add("players", teacup::route_mouse(m.players, players_view, "players"));
    m.events.add("input", teacup::route_keys(m.input, input_view));
    m.events.add("input", teacup::route_mouse(m.input, input_view, "input"));
    // Establish the ring from the tree once; the menu hub starts focused.
    if (m.events.focus().order().empty())
    {
        m.events.focus_order(areas.focus_order());
        m.events.focus().set("menu");
    }

    if (const auto* mouse = teacup::msg_cast<teacup::mouse_event>(message))
    {
        m.last_mouse = std::to_string(mouse->position.x) + "," + std::to_string(mouse->position.y);
        m.events.route(message, areas);
        if (menu_view.update(m.menu, *mouse, areas.get("menu")))
        {
            activate_menu();  // clicking a menu row jumps focus to it
        }
        viewport_view.update(m.viewport, *mouse, areas.get("players"));
        if (mouse->action == teacup::mouse_action::press && mouse->button == teacup::mouse_button::left)
        {
            if (const std::string* target = areas.hit(mouse->position);
                target != nullptr && *target == "players")
            {
                const int row = m.players.offset * 3 + mouse->position.y - areas.get("players").y;
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

    if (const auto* resize = teacup::msg_cast<teacup::resize_event>(message))
    {
        m.screen_size = resize->size;
        return teacup::next(m);
    }

    if (message.is<tick_message>())
    {
        ++m.tick;
        ++m.spinner.frame;
        return teacup::next(m, tick());
    }

    if (const auto* paste = teacup::msg_cast<teacup::paste_event>(message))
    {
        if (m.events.focus().is("input"))
        {
            input_view.update(m.input, *paste);
        }

        return teacup::next(m);
    }

    if (const auto* key = teacup::msg_cast<teacup::key_event>(message))
    {
        m.last_key = key->name();
        if (key->matches("ctrl+c"))
        {
            return teacup::next(m, teacup::quit());
        }

        const std::string was_focused = m.events.focus().focused();
        const bool consumed = m.events.route(message, areas);

        if (key->code == teacup::key::enter)
        {
            // Enter on the hub activates the selection; enter on a focused
            // widget backs out to the hub, leaving the selection on the
            // widget just left so it is easy to re-enter.
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

        // Page keys scroll the demo viewport regardless of focus.
        if (key->code == teacup::key::page_up)
        {
            if (was_focused == "players")
                players_view.page_up(m.players, areas.get("players").height);
            else
                viewport_view.page_up(m.viewport, 4);
            return teacup::next(m);
        }
        if (key->code == teacup::key::page_down)
        {
            if (was_focused == "players")
                players_view.page_down(m.players, areas.get("players").height);
            else
                viewport_view.page_down(m.viewport, 4);
            return teacup::next(m);
        }

        // Typing while the menu hub points at "text input" starts editing.
        if (!consumed && m.events.focus().is("menu") && m.menu.selected == 1
            && (key->code == teacup::key::character || key->code == teacup::key::space || key->code == teacup::key::backspace
                || key->code == teacup::key::del || key->code == teacup::key::home || key->code == teacup::key::end))
        {
            m.events.focus().set("input");
            sync_menu_to_focus();
            input_view.update(m.input, *key);
            return teacup::next(m);
        }
    }
    return teacup::next(m);
}

void view(const model& m, teacup::canvas screen)
{
    using namespace teacup;

    screen.fill(" ", style{}.bg(palette::surface()));
    const layout::areas areas = layout::resolve(showcase_tree(), screen.bounds());

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::accent()))
        .title(" teacup widget gallery ")
        .title_style(style{}.fg(palette::accent()).bold())
        .title_align(align::center)
        .padding(1)
        .render(
            screen.sub(areas.get("header")),
            [&](canvas)
            {
                widgets::text{ "A small, living tour of the teacup toolkit" }
                    .style(style{}.fg(palette::text()).bold())
                    .render(screen.sub(areas.get("title")));
                widgets::spinner::view_type{}
                    .frames({ "⠋ live",
                              "⠙ live",
                              "⠹ live",
                              "⠸ live",
                              "⠼ live",
                              "⠴ live",
                              "⠦ live",
                              "⠧ live",
                              "⠇ live",
                              "⠏ live" })
                    .style(style{}.fg(palette::accent()))
                    .render(m.spinner, screen.sub(areas.get("spinner")));
                widgets::tabs::view_type{}
                    .labels({ "gallery", "layout", "events" })
                    .tab_style(style{}.fg(palette::subtle()))
                    .selected_style(style{}.fg(palette::text()).bg(palette::accent()).bold())
                    .render(m.tabs, screen.sub(areas.get("tabs")));
            });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" navigation · players offset: " + std::to_string(m.players.offset) + " ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(areas.get("navigation")),
            [&](canvas)
            {
                widgets::list<std::string>::view_type{}
                    .selected_style(
                        !m.events.focus().is("menu")
                            ? style{}.fg(palette::accent()).bold()
                            : style{}.fg(palette::surface()).bg(palette::accent()).bold())
                    .item_style(style{}.fg(palette::text()))
                    .render(m.menu, screen.sub(areas.get("menu")));

                widgets::list<Player>::view_type{}
                    .item_height(3)
                    .selected_style(style{}.bg(color::hex("#313244")))
                    .item_style(style{}.fg(palette::text()))
                    .item_renderer(render_player)
                    .render(m.players, screen.sub(areas.get("players")));
            });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" components ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(areas.get("components")),
            [&](canvas)
            {
                widgets::progress::view_type{}
                    .gradient(palette::accent(), color::hex("#43bf6d"))
                    .render(widgets::progress::state_type{ (m.tick % 100) / 99.0 },
                            screen.sub(areas.get("progress")));

                widgets::table{}
                    .headers({ "widget", "state" })
                    .rows({ { "list", "ready" }, { "viewport", "scrolling" }, { "tabs", "selectable" } })
                    .header_style(style{}.fg(palette::accent()).bold())
                    .row_style(style{}.fg(palette::text()))
                    .render(screen.sub(areas.get("table")));

                widgets::block{}
                    .border(border::plain())
                    .border_style(style{}.fg(palette::subtle()))
                    .title(m.events.focus().is("input") ? " edit me [active] " : " edit me ")
                    .title_style(style{}.fg(m.events.focus().is("input") ? palette::accent() : palette::subtle()))
                    .padding(0)
                    .render(
                        screen.sub(areas.get("input-box")),
                        [&](canvas)
                        {
                            const canvas input_area = screen.sub(areas.get("input"));
                            widgets::text_input::view_type{}
                                .text_style(style{}.fg(palette::text()))
                                .cursor_style(
                                    m.events.focus().is("input") ? style{}.fg(palette::surface()).bg(palette::accent())
                                                    : style{}.fg(palette::subtle()).bg(palette::surface()))
                                .render(m.input, input_area.sub(rect{ 0, 0, input_area.width(), 1 }));
                            widgets::text{ "> " + m.input.value }
                                .style(style{}.fg(palette::subtle()))
                                .render(input_area.sub(rect{ 0, 1, input_area.width(), 1 }));
                            widgets::text{ "mouse: " + m.last_mouse }
                                .style(style{}.fg(palette::subtle()))
                                .render(input_area.sub(rect{ 0, 2, input_area.width(), 1 }));
                        });
            });

    widgets::text{ "arrows select · enter activate · tab/shift+tab focus · ctrl+c quit" }
        .style(style{}.fg(palette::surface()).bg(palette::accent()))
        .align(align::center)
        .render(screen.sub(areas.get("footer")));
}

}  // namespace

int main()
{
    model initial;
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

    teacup::options options;
    options.alt_screen = true;
    options.mouse = true;
    return teacup::run(teacup::make_app(initial, init, update, view), options);
}