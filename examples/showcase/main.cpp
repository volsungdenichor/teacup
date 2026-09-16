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
    bool input_focused = false;
    bool players_focused = false;
    bool tabs_focused = false;
    std::string last_key = "-";
    std::string last_mouse = "-";
    teacup::widgets::list<std::string>::state_type menu;
    teacup::widgets::tabs::state_type tabs;
    teacup::widgets::text_input::state_type input;
    teacup::widgets::viewport::state_type viewport;
    teacup::widgets::spinner::state_type spinner;
    teacup::widgets::list<Player>::state_type players;
};

struct showcase_layout
{
    teacup::rect tabs;
    teacup::rect menu;
    teacup::rect viewport;
    teacup::rect input;
};

showcase_layout layout_for(teacup::size screen_size)
{
    using namespace teacup;
    const rect screen = rect::from_size(screen_size);
    const auto rows = split_vertical(screen, { constraint::fixed(7), constraint::flex(), constraint::fixed(1) });
    const auto columns = split_horizontal(rows[1], { constraint::percent(42), constraint::flex() }, 1);
    const rect header_inner = rows[0].inset(2);
    const rect navigation_inner = columns[0].inset(2);
    const rect components_inner = columns[1].inset(2);
    const auto navigation_areas = split_vertical(navigation_inner, { constraint::fixed(6), constraint::flex() }, 1);
    const auto component_areas
        = split_vertical(components_inner, { constraint::fixed(1), constraint::fixed(4), constraint::fixed(5) }, 1);
    return { rect{ header_inner.x, header_inner.y + 2, header_inner.width, 1 },
             navigation_areas[0],
             navigation_areas[1],
             component_areas[2].inset(1) };
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
    const auto focus_menu = [&]
    {
        m.input_focused = false;
        m.players_focused = false;
        m.tabs_focused = false;
    };
    const auto focus_input = [&]
    {
        m.input_focused = true;
        m.players_focused = false;
        m.tabs_focused = false;
    };
    const auto focus_players = [&]
    {
        m.input_focused = false;
        m.players_focused = true;
        m.tabs_focused = false;
    };
    const auto focus_tabs = [&]
    {
        m.input_focused = false;
        m.players_focused = false;
        m.tabs_focused = true;
    };
    players_view.item_height(3).viewport_height(layout_for(m.screen_size).viewport.height);

    if (const auto* mouse = teacup::msg_cast<teacup::mouse_event>(message))
    {
        const auto layout = layout_for(m.screen_size);
        m.last_mouse = std::to_string(mouse->position.x) + "," + std::to_string(mouse->position.y);
        if (menu_view.update(m.menu, *mouse, layout.menu))
        {
            if (m.menu.selected == 1)
                focus_input();
            else if (m.menu.selected == 2)
                focus_players();
            else
                focus_menu();
        }
        if (tabs_view.labels({ "gallery", "layout", "events" }).update(m.tabs, *mouse, layout.tabs))
        {
            focus_tabs();
        }
        viewport_view.update(m.viewport, *mouse, layout.viewport);
        if (players_view.update(m.players, *mouse, layout.viewport))
        {
            focus_players();
            m.menu.selected = 2;
        }
        if (mouse->action == teacup::mouse_action::press && mouse->button == teacup::mouse_button::left
            && layout.viewport.contains(mouse->position))
        {
            const int row = m.players.offset * 3 + mouse->position.y - layout.viewport.y;
            const int item = row / 3;
            if (row % 3 == 2 && item >= 0 && item < static_cast<int>(m.players.items.size()))
            {
                m.players.items[static_cast<std::size_t>(item)].living
                    = !m.players.items[static_cast<std::size_t>(item)].living;
            }
        }
        if (input_view.update(m.input, *mouse, layout.input))
        {
            focus_input();
            m.menu.selected = 1;
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
        if (m.input_focused)
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

        if (key->code == teacup::key::tab && teacup::has(key->modifiers, teacup::modifier::shift))
        {
            if (m.tabs_focused)
                focus_players();
            else if (m.input_focused)
                focus_menu();
            else if (m.players_focused)
                focus_input();
            else
                focus_tabs();
            return teacup::next(m);
        }
        if (key->code == teacup::key::tab)
        {
            if (m.tabs_focused)
                focus_menu();
            else if (m.input_focused)
                focus_players();
            else if (m.players_focused)
                focus_tabs();
            else
                focus_input();
            if (m.input_focused)
                m.menu.selected = 1;
            if (m.players_focused)
                m.menu.selected = 2;
            return teacup::next(m);
        }

        if (m.input_focused)
        {
            if (key->code == teacup::key::enter)
            {
                focus_menu();
                return teacup::next(m);
            }
            input_view.update(m.input, *key);
            return teacup::next(m);
        }

        if (m.tabs_focused)
        {
            if (key->code == teacup::key::left || key->code == teacup::key::right)
            {
                tabs_view.update(m.tabs, *key);
                return teacup::next(m);
            }
            if (key->code == teacup::key::enter)
            {
                focus_menu();
                return teacup::next(m);
            }
        }

        if (m.players_focused)
        {
            if (key->code == teacup::key::up || key->code == teacup::key::down || key->code == teacup::key::home
                || key->code == teacup::key::end)
            {
                players_view.update(m.players, *key);
                return teacup::next(m);
            }
            const int height = layout_for(m.screen_size).viewport.height;
            if (key->code == teacup::key::page_up)
            {
                players_view.page_up(m.players, height);
                return teacup::next(m);
            }
            if (key->code == teacup::key::page_down)
            {
                players_view.page_down(m.players, height);
                return teacup::next(m);
            }
            if (key->code == teacup::key::enter)
            {
                focus_menu();
                return teacup::next(m);
            }
        }

        if (key->code == teacup::key::enter)
        {
            if (m.menu.selected == 1)
                focus_input();
            else if (m.menu.selected == 2)
                focus_players();
            else
                focus_menu();
            return teacup::next(m);
        }

        if (m.menu.selected == 1
            && (key->code == teacup::key::character || key->code == teacup::key::space || key->code == teacup::key::backspace
                || key->code == teacup::key::del || key->code == teacup::key::home || key->code == teacup::key::end))
        {
            focus_input();
            input_view.update(m.input, *key);
            return teacup::next(m);
        }

        if (menu_view.update(m.menu, *key))
        {
            if (m.menu.selected == 1)
                focus_input();
            else if (m.menu.selected == 2)
                focus_players();
            else
                focus_menu();
            return teacup::next(m);
        }

        if (key->code == teacup::key::page_up)
        {
            viewport_view.page_up(m.viewport, 4);
            return teacup::next(m);
        }
        if (key->code == teacup::key::page_down)
        {
            viewport_view.page_down(m.viewport, 4);
            return teacup::next(m);
        }
    }
    return teacup::next(m);
}

void heading(teacup::canvas target, const std::string& title, const teacup::color& color)
{
    teacup::widgets::text{ title }.style(teacup::style{}.fg(color).bold()).render(target);
}

void view(const model& m, teacup::canvas screen)
{
    using namespace teacup;

    screen.fill(" ", style{}.bg(palette::surface()));
    const auto rows = split_vertical(screen.bounds(), { constraint::fixed(7), constraint::flex(), constraint::fixed(1) });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::accent()))
        .title(" teacup widget gallery ")
        .title_style(style{}.fg(palette::accent()).bold())
        .title_align(align::center)
        .padding(1)
        .render(
            screen.sub(rows[0]),
            [&](canvas inner)
            {
                const auto header = split_horizontal(inner.bounds(), { constraint::flex(), constraint::fixed(14) });
                widgets::text{ "A small, living tour of the teacup toolkit" }
                    .style(style{}.fg(palette::text()).bold())
                    .render(inner.sub(header[0]));
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
                    .render(m.spinner, inner.sub(header[1]));
                widgets::tabs::view_type{}
                    .labels({ "gallery", "layout", "events" })
                    .tab_style(style{}.fg(palette::subtle()))
                    .selected_style(style{}.fg(palette::text()).bg(palette::accent()).bold())
                    .render(m.tabs, inner.sub(rect{ 0, 2, inner.width(), 1 }));
            });

    const auto columns = split_horizontal(rows[1], { constraint::percent(42), constraint::flex() }, 1);

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" navigation · players offset: " + std::to_string(m.players.offset) + " ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(columns[0]),
            [&](canvas inner)
            {
                const auto areas = split_vertical(inner.bounds(), { constraint::fixed(6), constraint::flex() }, 1);
                widgets::list<std::string>::view_type{}
                    .selected_style(
                        m.input_focused || m.players_focused || m.tabs_focused
                            ? style{}.fg(palette::accent()).bold()
                            : style{}.fg(palette::surface()).bg(palette::accent()).bold())
                    .item_style(style{}.fg(palette::text()))
                    .render(m.menu, inner.sub(areas[0]));

                widgets::list<Player>::view_type{}
                    .item_height(3)
                    .selected_style(style{}.bg(color::hex("#313244")))
                    .item_style(style{}.fg(palette::text()))
                    .item_renderer(render_player)
                    .render(m.players, inner.sub(areas[1]));
            });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" components ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(
            screen.sub(columns[1]),
            [&](canvas inner)
            {
                const auto areas = split_vertical(
                    inner.bounds(), { constraint::fixed(1), constraint::fixed(4), constraint::fixed(5) }, 1);

                widgets::progress::view_type{}
                    .gradient(palette::accent(), color::hex("#43bf6d"))
                    .render(widgets::progress::state_type{ (m.tick % 100) / 99.0 }, inner.sub(areas[0]));

                widgets::table{}
                    .headers({ "widget", "state" })
                    .rows({ { "list", "ready" }, { "viewport", "scrolling" }, { "tabs", "selectable" } })
                    .header_style(style{}.fg(palette::accent()).bold())
                    .row_style(style{}.fg(palette::text()))
                    .render(inner.sub(areas[1]));

                widgets::block{}
                    .border(border::plain())
                    .border_style(style{}.fg(palette::subtle()))
                    .title(m.input_focused ? " edit me [active] " : " edit me ")
                    .title_style(style{}.fg(m.input_focused ? palette::accent() : palette::subtle()))
                    .padding(0)
                    .render(
                        inner.sub(areas[2]),
                        [&](canvas input_area)
                        {
                            widgets::text_input::view_type{}
                                .text_style(style{}.fg(palette::text()))
                                .cursor_style(
                                    m.input_focused ? style{}.fg(palette::surface()).bg(palette::accent())
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
        .render(screen.sub(rows[2]));
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