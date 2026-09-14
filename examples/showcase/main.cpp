#include <chrono>
#include <algorithm>
#include <string>
#include <vector>

#include "teacup/teacup.hpp"

namespace
{

struct tick_message
{
};

struct model
{
    int tick = 0;
    bool input_focused = false;
    std::string last_key = "-";
    teacup::widgets::list_state menu;
    teacup::widgets::tabs_state tabs;
    teacup::widgets::text_input_state input;
    teacup::widgets::viewport_state viewport;
    teacup::widgets::spinner_state spinner;
};

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
    const teacup::widgets::list_view menu_view;
    const teacup::widgets::text_input_view input_view;
    const teacup::widgets::viewport_view viewport_view;

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
            m.input_focused = !m.input_focused;
            if (m.input_focused)
            {
                m.menu.selected = 1;
            }
            return teacup::next(m);
        }
        if (key->code == teacup::key::tab)
        {
            m.input_focused = !m.input_focused;
            if (m.input_focused)
            {
                m.menu.selected = 1;
            }
            return teacup::next(m);
        }

        if (m.input_focused)
        {
            if (key->code == teacup::key::up || key->code == teacup::key::down)
            {
                if (menu_view.update(m.menu, *key))
                {
                    m.input_focused = m.menu.selected == 1;
                    return teacup::next(m);
                }
            }
            if (key->code == teacup::key::enter)
            {
                m.input_focused = false;
                return teacup::next(m);
            }
            input_view.update(m.input, *key);
            return teacup::next(m);
        }

        if (key->code == teacup::key::enter)
        {
            m.input_focused = m.menu.selected == 1;
            return teacup::next(m);
        }

        if (m.menu.selected == 1 && (key->code == teacup::key::character
                                     || key->code == teacup::key::space
                                     || key->code == teacup::key::backspace
                                     || key->code == teacup::key::del
                                     || key->code == teacup::key::home
                                     || key->code == teacup::key::end))
        {
            m.input_focused = true;
            input_view.update(m.input, *key);
            return teacup::next(m);
        }

        if (menu_view.update(m.menu, *key))
        {
            m.input_focused = m.menu.selected == 1;
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
    teacup::widgets::text{ title }
        .style(teacup::style{}.fg(color).bold())
        .render(target);
}

void view(const model& m, teacup::canvas screen)
{
    using namespace teacup;

    screen.fill(" ", style{}.bg(palette::surface()));
    const auto rows = split_vertical(
        screen.bounds(), { constraint::fixed(4), constraint::flex(), constraint::fixed(1) });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::accent()))
        .title(" teacup widget gallery ")
        .title_style(style{}.fg(palette::accent()).bold())
        .title_align(align::center)
        .padding(1)
        .render(screen.sub(rows[0]), [&](canvas inner) {
            const auto header = split_horizontal(inner.bounds(), { constraint::flex(), constraint::fixed(14) });
            widgets::text{ "A small, living tour of the teacup toolkit" }
                .style(style{}.fg(palette::text()).bold())
                .render(inner.sub(header[0]));
            widgets::spinner_view{}
                .frames({ "| ready", "/ ready", "- ready", "\\ ready" })
                .style(style{}.fg(palette::accent()))
                .render(m.spinner, inner.sub(header[1]));
            widgets::tabs_view{}
                .labels({ "gallery", "layout", "events" })
                .tab_style(style{}.fg(palette::subtle()))
                .selected_style(style{}.fg(palette::text()).bg(palette::accent()).bold())
                .render(m.tabs, inner.sub(rect{ 0, 2, inner.width(), 1 }));
        });

    const auto columns = split_horizontal(rows[1], { constraint::percent(42), constraint::flex() }, 1);

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" navigation ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(screen.sub(columns[0]), [&](canvas inner) {
            const auto areas = split_vertical(inner.bounds(), { constraint::fixed(6), constraint::flex() }, 1);
            widgets::list_view{}
                .selected_style(m.input_focused
                        ? style{}.fg(palette::accent()).bold()
                        : style{}.fg(palette::surface()).bg(palette::accent()).bold())
                .item_style(style{}.fg(palette::text()))
                .render(m.menu, inner.sub(areas[0]));

            widgets::viewport_view{}
                .style(style{}.fg(palette::subtle()))
                .render(m.viewport, inner.sub(areas[1]));
        });

    widgets::block{}
        .border(border::rounded())
        .border_style(style{}.fg(palette::subtle()))
        .title(" components ")
        .title_style(style{}.fg(palette::subtle()))
        .padding(1)
        .render(screen.sub(columns[1]), [&](canvas inner) {
            const auto areas = split_vertical(
                inner.bounds(), { constraint::fixed(1), constraint::fixed(4), constraint::fixed(5) }, 1);

            widgets::progress{ (m.tick % 100) / 99.0 }
                .gradient(palette::accent(), color::hex("#43bf6d"))
                .render(inner.sub(areas[0]));

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
                .render(inner.sub(areas[2]), [&](canvas input_area) {
                    widgets::text_input_view{}
                        .text_style(style{}.fg(palette::text()))
                        .cursor_style(m.input_focused
                                ? style{}.fg(palette::surface()).bg(palette::accent())
                                : style{}.fg(palette::subtle()).bg(palette::surface()))
                        .render(m.input, input_area.sub(rect{ 0, 0, input_area.width(), 1 }));
                    widgets::text{ "> " + m.input.value }
                        .style(style{}.fg(palette::subtle()))
                        .render(input_area.sub(rect{ 0, 1, input_area.width(), 1 }));
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
    initial.menu.items = { "overview", "text input", "viewport", "table", "progress" };
    initial.tabs.selected = 0;
    initial.input.value = "type here";
    initial.input.cursor = initial.input.value.size();
    initial.viewport.lines = { "viewport", "scrollable", "content", "lives", "inside", "a", "small", "window" };

    teacup::options options;
    options.alt_screen = true;
    return teacup::run(teacup::make_app(initial, init, update, view), options);
}