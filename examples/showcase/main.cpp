#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "teacup/teacup.hpp"

namespace
{

struct TickMessage
{
};

template <class T, class Pred, class = void>
struct IsEqualityComparable : std::false_type
{
};

template <class T, class Pred>
struct IsEqualityComparable<T, Pred, std::void_t<decltype(std::declval<const T&>() == std::declval<const Pred&>())>>
    : std::is_convertible<decltype(std::declval<const T&>() == std::declval<const Pred&>()), bool>
{
};

template <class Pred, class T>
bool matches(const T& item, const Pred& pred)
{
    if constexpr (IsEqualityComparable<T, Pred>::value)
    {
        return item == pred;
    }
    else if constexpr (std::is_invocable_r_v<bool, const Pred&, const T&>)
    {
        return std::invoke(pred, item);
    }
    else
    {
        static_assert(
            std::is_invocable_r_v<bool, const Pred&, const T&>,
            "matches requires equality comparison or a predicate returning a value convertible to bool");
    }
}

struct Any
{
    template <class... Preds>
    struct Impl
    {
        std::tuple<Preds...> preds;

        template <class T>
        bool operator()(const T& item) const
        {
            return std::apply([&item](const Preds&... preds) { return (matches(item, preds) || ...); }, preds);
        }

        template <class T>
        bool friend operator|(const T& item, const Impl& impl)
        {
            return impl(item);
        }
    };

    template <class... Preds>
    auto operator()(Preds... preds) const
    {
        return Impl<Preds...>{ std::make_tuple(std::move(preds)...) };
    }
};

constexpr inline auto any = Any{};

enum class Country
{
    usa,
    czech_republic,
    france,
    poland,
    italy,
    uk,
    sweden,
};

struct Person
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
    teacup::widgets::DrawingArea::State drawing;
    teacup::widgets::Spinner::State spinner;
    teacup::widgets::List<Person>::State people;
};

teacup::widgets::Block panel_block()
{
    return teacup::widgets::Block{}.border(teacup::Border::rounded()).padding(1);
}

teacup::widgets::Block input_block()
{
    return teacup::widgets::Block{}.border(teacup::Border::plain()).padding(0);
}

teacup::layout::Node block_layout(
    std::string id, teacup::widgets::Block block, teacup::layout::Node content)
{
    return teacup::layout::named(
        std::move(id),
        teacup::layout::within(
            [block = std::move(block)](const teacup::Rect& area) { return block.inner(area); },
            std::move(content)));
}

teacup::layout::Node showcase_tree()
{
    using namespace teacup::layout;
    return column(
        {
            fixed(
                7,
                block_layout(
                    "header",
                    panel_block(),
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
                        }))),
            flex(row(
                {
                    percent(
                        42,
                        block_layout(
                            "navigation",
                            panel_block(),
                            column(
                                {
                                    fixed(6, focusable("menu")),
                                    flex(focusable("people")),
                                },
                                1))),
                    flex(block_layout(
                        "components",
                        panel_block(),
                        column(
                            {
                                fixed(1, leaf("progress")),
                                flex(row(
                                    {
                                        percent(50, leaf("table")),
                                        flex(block_layout("drawing-box", input_block(), focusable("drawing"))),
                                    },
                                    1)),
                                fixed(5, block_layout(
                                    "input-box",
                                    input_block(),
                                    column({
                                        fixed(1, focusable("input")),
                                        flex(space()),
                                    }))),
                            },
                            1))),
                },
                1)),
            fixed(1, leaf("footer")),
        });
}

void render_player(
    const Person& person, teacup::Canvas row, const teacup::Style&, const teacup::widgets::ListItemState& state)
{
    using namespace teacup;
    widgets::Text{ person.name }
        .style(Style{}.fg(state.selected ? palette::accent() : palette::text()).bold())
        .render(row.horizontal_slice(0));
    widgets::Text{ "Country: " + country_name(person.country) }
        .style(Style{}.fg(palette::subtle()))
        .render(row.horizontal_slice(1));

    widgets::Text{ "Living:" }.style(Style{}.fg(palette::subtle())).render(row.horizontal_slice(2));
    widgets::Checkbox::View{}
        .checked_style(Style{}.fg(palette::success()).bold())
        .unchecked_style(Style{}.fg(palette::danger()))
        .render(widgets::Checkbox::State{ person.living }, row.horizontal_slice(2).indent(8));
}

constexpr teacup::Size drawing_content_size{ 72, 24 };

// Drawn in content coordinates; the drawing area scrolls this scene inside its window.
void draw_scene(teacup::Canvas c)
{
    using namespace teacup;
    const Size size = c.size();
    for (int y = 0; y < size.height; y += 4)
    {
        for (int x = 0; x < size.width; ++x)
        {
            c.put(Point{ x, y }, Cell{ Glyph{ x % 8 == 0 ? "+" : "·" }, Style{}.fg(palette::subtle()).dim() });
        }
        for (int x = 0; x < size.width; x += 8)
        {
            c.text(Point{ x + 1, y }, std::to_string(x) + "," + std::to_string(y), Style{}.fg(palette::subtle()));
        }
    }

    widgets::Block{}
        .border(Border::rounded())
        .border_style(Style{}.fg(palette::accent()))
        .title(" origin ")
        .title_style(Style{}.fg(palette::accent()).bold())
        .render(
            c.sub(Rect{ 2, 1, 20, 5 }),
            [](Canvas inner)
            {
                widgets::Text{ "scroll me: wheel," }.style(Style{}.fg(palette::text())).render(inner.horizontal_slice(0));
                widgets::Text{ "shift+wheel, arrows" }.style(Style{}.fg(palette::text())).render(inner.horizontal_slice(1));
                widgets::Text{ "pgup/pgdn, home/end" }.style(Style{}.fg(palette::text())).render(inner.horizontal_slice(2));
            });

    for (int x = 0; x < size.width; ++x)
    {
        const double t = static_cast<double>(x) / (std::max)(1, size.width - 1);
        c.put(Point{ x, 10 }, Cell{ Glyph{ "▀" }, Style{}.fg(Color::mix(palette::accent(), palette::success(), t)) });
    }

    widgets::Block{}
        .border(Border::thick())
        .border_style(Style{}.fg(palette::warning()))
        .title(" far east ")
        .render(c.sub(Rect{ 44, 13, 24, 5 }), [](Canvas inner) { inner.text(Point{ 1, 1 }, "東の果て 🍵", Style{}.fg(palette::text())); });

    c.text(
        Point{ size.width - 22, size.height - 1 }, "you found the corner!", Style{}.fg(palette::danger()).bold());
}

teacup::widgets::DrawingArea::View drawing_view(const teacup::Size& viewport)
{
    return teacup::widgets::DrawingArea::View{}
        .content_size(drawing_content_size)
        .viewport_size(viewport)
        .background(teacup::Style{}.bg(teacup::palette::surface()))
        .draw(draw_scene);
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
    teacup::widgets::List<Person>::View people_view;
    const teacup::layout::Areas areas = teacup::layout::resolve(showcase_tree(), teacup::Rect::from_size(m.screen_size));
    people_view.item_height(3).viewport_height(areas.get("people").height());
    const teacup::widgets::DrawingArea::View scene_view = drawing_view(areas.get("drawing").size());

    const auto sync_menu_to_focus = [&]
    {
        const std::string& f = m.events.focus().focused();
        m.menu.selected = f == "input" ? 1 : f == "people" ? 2 : 0;
    };
    const auto activate_menu = [&]
    {
        m.events.focus().set(m.menu.selected == 1 ? "input" : m.menu.selected == 2 ? "people" : "menu");
    };

    m.events.clear();
    m.events.add("menu", teacup::route_keys(m.menu, menu_view));
    m.events.add("tabs", teacup::route_keys(m.tabs, tabs_view));
    m.events.add("tabs", teacup::route_mouse(m.tabs, tabs_view.labels({ "gallery", "layout", "events" }), "tabs"));
    m.events.add("people", teacup::route_keys(m.people, people_view));
    m.events.add("people", teacup::route_mouse(m.people, people_view, "people"));
    m.events.add("input", teacup::route_keys(m.input, input_view));
    m.events.add("input", teacup::route_mouse(m.input, input_view, "input"));
    m.events.add("drawing", teacup::route_keys(m.drawing, scene_view));
    m.events.add("drawing", teacup::route_mouse(m.drawing, scene_view, "drawing"));

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
        viewport_view.update(m.viewport, *mouse, areas.get("people"));
        if (mouse->action == teacup::MouseAction::press && mouse->button == teacup::MouseButton::left)
        {
            const teacup::Rect area = areas.get("people");
            const int item = people_view.item_at(m.people, mouse->position, area);
            if (item >= 0 && (mouse->position.y - area.top()) % 3 == 2)
            {
                m.people.items[static_cast<std::size_t>(item)].living
                    = !m.people.items[static_cast<std::size_t>(item)].living;
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
                m.menu.selected = was_focused == "input" ? 1 : was_focused == "people" ? 2 : 0;
                m.events.focus().set("menu");
            }
            return teacup::next(m);
        }

        if (consumed && was_focused == "drawing")
        {
            return teacup::next(m);
        }

        if (key->code == teacup::Key::page_up)
        {
            if (was_focused == "people")
            {
                people_view.page_up(m.people, areas.get("people").height());
            }
            else
            {
                viewport_view.page_up(m.viewport, 4);
            }
            return teacup::next(m);
        }
        if (key->code == teacup::Key::page_down)
        {
            if (was_focused == "people")
            {
                people_view.page_down(m.people, areas.get("people").height());
            }
            else
            {
                viewport_view.page_down(m.viewport, 4);
            }
            return teacup::next(m);
        }

        if (!consumed && m.events.focus().is("menu") && m.menu.selected == 1
            && (key->code
                | any(
                    teacup::Key::character,
                    teacup::Key::space,
                    teacup::Key::backspace,
                    teacup::Key::del,
                    teacup::Key::home,
                    teacup::Key::end)))
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

    panel_block()
        .border_style(Style{}.fg(palette::accent()))
        .title(" teacup widget gallery ")
        .title_style(Style{}.fg(palette::accent()).bold())
        .title_align(Align::center)
        .render(
            screen.sub(areas.get("header")),
            [&](Canvas content)
            {
                widgets::Text{ "A small, living tour of the teacup toolkit" }
                    .style(Style{}.fg(palette::text()).bold())
                    .render(content.sub_absolute(areas.get("title")));
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
                    .render(m.spinner, content.sub_absolute(areas.get("spinner")));
                widgets::Tabs::View{}
                    .labels({ "gallery", "layout", "events" })
                    .tab_style(Style{}.fg(palette::subtle()))
                    .selected_style(Style{}.fg(palette::text()).bg(palette::accent()).bold())
                    .render(m.tabs, content.sub_absolute(areas.get("tabs")));
            });

    panel_block()
        .border_style(Style{}.fg(palette::subtle()))
        .title(" navigation · people offset: " + std::to_string(m.people.offset) + " ")
        .title_style(Style{}.fg(palette::subtle()))
        .render(
            screen.sub(areas.get("navigation")),
            [&](Canvas content)
            {
                widgets::List<std::string>::View{}
                    .selected_style(
                        !m.events.focus().is("menu") ? Style{}.fg(palette::accent()).bold()
                                                     : Style{}.fg(palette::surface()).bg(palette::accent()).bold())
                    .item_style(Style{}.fg(palette::text()))
                    .render(m.menu, content.sub_absolute(areas.get("menu")));

                widgets::List<Person>::View{}
                    .item_height(3)
                    .selected_style(Style{}.bg(Color::hex("#313244")))
                    .item_style(Style{}.fg(palette::text()))
                    .item_renderer(render_player)
                    .render(m.people, content.sub_absolute(areas.get("people")));
            });

    panel_block()
        .border_style(Style{}.fg(palette::subtle()))
        .title(" components ")
        .title_style(Style{}.fg(palette::subtle()))
        .render(
            screen.sub(areas.get("components")),
            [&](Canvas content)
            {
                widgets::Progress::View{}
                    .gradient(palette::accent(), Color::hex("#43bf6d"))
                    .render(
                        widgets::Progress::State{ (m.tick % 100) / 99.0 },
                        content.sub_absolute(areas.get("progress")));

                widgets::Table{}
                    .headers({ "widget", "state" })
                    .rows({ { "list", "ready" }, { "viewport", "scrolling" }, { "tabs", "selectable" } })
                    .header_style(Style{}.fg(palette::accent()).bold())
                    .row_style(Style{}.fg(palette::text()))
                    .render(content.sub_absolute(areas.get("table")));

                const bool drawing_focused = m.events.focus().is("drawing");
                const Point scroll = drawing_view(areas.get("drawing").size())
                                         .visible_offset(m.drawing, areas.get("drawing").size());
                input_block()
                    .border_style(Style{}.fg(drawing_focused ? palette::accent() : palette::subtle()))
                    .title(" drawing " + std::to_string(scroll.x) + "," + std::to_string(scroll.y) + " ")
                    .title_style(Style{}.fg(drawing_focused ? palette::accent() : palette::subtle()))
                    .render(
                        content.sub_absolute(areas.get("drawing-box")),
                        [&](Canvas drawing_area)
                        {
                            drawing_view(drawing_area.size()).render(m.drawing, drawing_area);
                        });

                input_block()
                    .border_style(Style{}.fg(palette::subtle()))
                    .title(m.events.focus().is("input") ? " edit me [active] " : " edit me ")
                    .title_style(Style{}.fg(m.events.focus().is("input") ? palette::accent() : palette::subtle()))
                    .render(
                        content.sub_absolute(areas.get("input-box")),
                        [&](Canvas input_area)
                        {
                            widgets::TextInput::View{}
                                .text_style(Style{}.fg(palette::text()))
                                .cursor_style(
                                    m.events.focus().is("input") ? Style{}.fg(palette::surface()).bg(palette::accent())
                                                                 : Style{}.fg(palette::subtle()).bg(palette::surface()))
                                .render(m.input, input_area.horizontal_slice(0));
                            widgets::Text{ "> " + m.input.value }
                                .style(Style{}.fg(palette::subtle()))
                                .render(input_area.horizontal_slice(1));
                            widgets::Text{ "mouse: " + m.last_mouse }
                                .style(Style{}.fg(palette::subtle()))
                                .render(input_area.horizontal_slice(2));
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
    Model model;
    model.menu.items = { "overview", "text input", "people", "table", "progress" };
    model.tabs.selected = 0;
    model.input.value = teacup::GlyphString::from_utf8("type here");
    model.input.cursor = model.input.value.size();
    model.viewport.lines = { "viewport", "scrollable", "content", "lives", "inside", "a", "small", "window" };
    model.people.items = {
        { "John Wayne", Country::usa, false },     { "Alain Delon", Country::france, false },
        { "Al Pacino", Country::usa, true },       { "Robert De Niro", Country::usa, true },
        { "Marilyn Monroe", Country::usa, false }, { "Sophia Loren", Country::italy, true },
        { "Clint Eastwood", Country::usa, true },  { "Charlton Heston", Country::usa, false },
        { "Audrey Hepburn", Country::uk, false },  { "Ingrid Bergman", Country::sweden, false },
    };

    teacup::Options options;
    options.alt_screen = true;
    options.mouse = true;
    return teacup::run(teacup::make_app(model, init, update, view), options);
}