#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
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

struct World
{
    static const inline std::array<std::string, 12> world_map = {
        "################",  //
        "#......#.......#",  //
        "#..2...#...3...#",  //
        "#......#.......#",  //
        "#.............1#",  //
        "###.####.......#",  //
        "#.......####.###",  //
        "#..33...#......#",  //
        "#.......#..2...#",  //
        "#...........1..#",  //
        "#.......#......#",  //
        "################",
    };

    static const inline int world_width = world_map[0].size();
    static const inline int world_height = static_cast<int>(world_map.size());
    static const inline double field_of_view_plane = 0.66;  // camera plane half-width; ~66 degree horizontal FOV
    static const inline double player_radius = 0.2;

    char at(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= world_width || y >= world_height)
        {
            return '#';
        }
        return world_map[static_cast<std::size_t>(y)][x];
    }

    bool is_wall(double x, double y) const
    {
        return at(static_cast<int>(std::floor(x)), static_cast<int>(std::floor(y))) != '.';
    }
};

struct Raycaster
{
    double x = 1.5;
    double y = 4.5;
    double angle = 0.15;  // radians; the map's y axis points down, so positive angles turn right
    bool show_map = false;

    void turn(double amount) { angle = std::remainder(angle + amount, 2.0 * M_PI); }

    // Moves with axis-separated collision so the player slides along walls.
    void move(double forward, double strafe, const World& world)
    {
        const double dx = std::cos(angle) * forward - std::sin(angle) * strafe;
        const double dy = std::sin(angle) * forward + std::cos(angle) * strafe;
        const auto margin = [&](double d) { return d > 0 ? world.player_radius : d < 0 ? -world.player_radius : 0.0; };
        if (!world.is_wall(x + dx + margin(dx), y))
        {
            x += dx;
        }
        if (!world.is_wall(x, y + dy + margin(dy)))
        {
            y += dy;
        }
    }
};

struct RayHit
{
    double distance = 0.0;  // perpendicular to the camera plane, which avoids fish-eye distortion
    char wall = '#';
    bool vertical_face = false;  // hit a wall face parallel to the y axis
    double face_offset = 0.0;    // 0..1 position of the hit along the wall face
};

// Classic DDA: step through the grid one cell boundary at a time until a wall is hit.
RayHit cast_ray(double px, double py, double ray_x, double ray_y, const World& world)
{
    int cell_x = static_cast<int>(std::floor(px));
    int cell_y = static_cast<int>(std::floor(py));
    const double delta_x = ray_x == 0.0 ? 1e30 : std::abs(1.0 / ray_x);
    const double delta_y = ray_y == 0.0 ? 1e30 : std::abs(1.0 / ray_y);
    const int step_x = ray_x < 0 ? -1 : 1;
    const int step_y = ray_y < 0 ? -1 : 1;
    double side_x = (ray_x < 0 ? px - cell_x : cell_x + 1.0 - px) * delta_x;
    double side_y = (ray_y < 0 ? py - cell_y : cell_y + 1.0 - py) * delta_y;

    RayHit hit;
    for (int i = 0; i < 64; ++i)
    {
        if (side_x < side_y)
        {
            side_x += delta_x;
            cell_x += step_x;
            hit.vertical_face = true;
        }
        else
        {
            side_y += delta_y;
            cell_y += step_y;
            hit.vertical_face = false;
        }
        hit.wall = world.at(cell_x, cell_y);
        if (hit.wall != '.')
        {
            break;
        }
    }
    hit.distance = (std::max)(1e-3, hit.vertical_face ? side_x - delta_x : side_y - delta_y);
    const double along = hit.vertical_face ? py + hit.distance * ray_y : px + hit.distance * ray_x;
    hit.face_offset = along - std::floor(along);
    return hit;
}

teacup::Color wall_color(char wall)
{
    switch (wall)
    {
        case '1': return teacup::Color::hex("#f38ba8");
        case '2': return teacup::Color::hex("#a6e3a1");
        case '3': return teacup::Color::hex("#f9e2af");
        default: return teacup::Color::hex("#89b4fa");
    }
}

teacup::Color shade_wall(const RayHit& hit)
{
    const teacup::Color fog = teacup::Color::hex("#11111b");
    teacup::Color c = wall_color(hit.wall);
    if (hit.vertical_face)
    {
        c = teacup::Color::mix(c, fog, 0.3);
    }
    if (hit.face_offset < 0.05 || hit.face_offset > 0.95)
    {
        c = teacup::Color::mix(c, fog, 0.5);  // dark seams between wall blocks
    }
    return teacup::Color::mix(c, fog, (std::min)(0.85, hit.distance / 10.0));
}

// Renders the first-person view into `c`, two square pixels per cell using upper half blocks.
void draw_raycaster(const Raycaster& r, teacup::Canvas c, const World& world)
{
    using namespace teacup;
    const int width = c.size().width;
    const int pixel_height = c.size().height * 2;
    if (width <= 0 || pixel_height <= 0)
    {
        return;
    }
    const double dir_x = std::cos(r.angle);
    const double dir_y = std::sin(r.angle);
    const double plane_x = -dir_y * world.field_of_view_plane;
    const double plane_y = dir_x * world.field_of_view_plane;
    const double focal = (width / 2.0) / world.field_of_view_plane;
    const double horizon = pixel_height / 2.0;

    const auto backdrop = [&](int py)
    {
        if (py < horizon)
        {
            return Color::mix(Color::hex("#313244"), Color::hex("#181825"), py / horizon);
        }
        return Color::mix(Color::hex("#181825"), Color::hex("#585b70"), (py - horizon) / horizon);
    };

    for (int x = 0; x < width; ++x)
    {
        const double camera = 2.0 * (x + 0.5) / width - 1.0;
        const RayHit hit = cast_ray(r.x, r.y, dir_x + plane_x * camera, dir_y + plane_y * camera, world);
        const double wall_height = focal / hit.distance;
        const double wall_top = horizon - wall_height / 2.0;
        const double wall_bottom = horizon + wall_height / 2.0;
        const Color wall = shade_wall(hit);
        const auto pixel = [&](int py) { return py + 0.5 >= wall_top && py + 0.5 < wall_bottom ? wall : backdrop(py); };
        for (int row = 0; row < c.size().height; ++row)
        {
            c.put(Point{ x, row }, Cell{ Glyph{ "▀" }, Style{}.fg(pixel(2 * row)).bg(pixel(2 * row + 1)) });
        }
    }

    if (r.show_map)
    {
        const int facing_x = static_cast<int>(std::floor(r.x + dir_x));
        const int facing_y = static_cast<int>(std::floor(r.y + dir_y));
        const auto map_pixel = [&](int mx, int my)
        {
            if (mx == static_cast<int>(r.x) && my == static_cast<int>(r.y))
            {
                return Color::hex("#ffffff");
            }
            const char cell = world.at(mx, my);
            if (cell != '.')
            {
                return Color::mix(wall_color(cell), Color::hex("#11111b"), 0.3);
            }
            return mx == facing_x && my == facing_y ? palette::accent() : Color::hex("#11111b");
        };
        for (int row = 0; row < (world.world_height + 1) / 2; ++row)
        {
            for (int mx = 0; mx < world.world_width; ++mx)
            {
                c.put(
                    Point{ mx, row },
                    Cell{ Glyph{ "▀" }, Style{}.fg(map_pixel(mx, 2 * row)).bg(map_pixel(mx, 2 * row + 1)) });
            }
        }
    }
}

// Applies a raycaster control key; returns false for keys it does not handle.
bool steer(Raycaster& r, const teacup::KeyEvent& key, const World& world)
{
    constexpr double step = 0.25;
    constexpr double turn = M_PI / 16.0;
    if (key.matches_any({ "w", "up" }))
    {
        r.move(step, 0.0, world);
    }
    else if (key.matches_any({ "s", "down" }))
    {
        r.move(-step, 0.0, world);
    }
    else if (key.matches_any({ "a", "left" }))
    {
        r.turn(-turn);
    }
    else if (key.matches_any({ "d", "right" }))
    {
        r.turn(turn);
    }
    else if (key.matches("q"))
    {
        r.move(0.0, -step, world);
    }
    else if (key.matches("e"))
    {
        r.move(0.0, step, world);
    }
    else if (key.matches("m"))
    {
        r.show_map = !r.show_map;
    }
    else
    {
        return false;
    }
    return true;
}

struct Model
{
    int tick = 0;
    teacup::Size screen_size{ 80, 24 };
    teacup::Router events;
    std::string last_key = "-";
    std::string last_mouse = "-";
    teacup::widgets::List<std::string>::State menu;
    teacup::widgets::Tabs::State tabs;
    teacup::widgets::Viewport::State viewport;
    teacup::widgets::DrawingArea::State drawing;
    Raycaster raycaster;
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

teacup::layout::Node block_layout(std::string id, teacup::widgets::Block block, teacup::layout::Node content)
{
    return teacup::layout::named(
        std::move(id),
        teacup::layout::within(
            [block = std::move(block)](const teacup::Rect& area) { return block.inner(area); }, std::move(content)));
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
                                flex(block_layout("drawing-box", input_block(), focusable("drawing"))),
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

// Below this size the 3D view keeps its resolution and the drawing area scrolls over it.
constexpr teacup::Size min_render_size{ 32, 8 };

const auto world = World{};

teacup::widgets::DrawingArea::View drawing_view(const Raycaster& raycaster, const teacup::Size& viewport)
{
    return teacup::widgets::DrawingArea::View{}
        .content_size(
            teacup::Size{ (std::max)(viewport.width, min_render_size.width),
                          (std::max)(viewport.height, min_render_size.height) })
        .viewport_size(viewport)
        .background(teacup::Style{}.bg(teacup::palette::surface()))
        .draw([&raycaster](teacup::Canvas c) { draw_raycaster(raycaster, c, world); });
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
    const teacup::widgets::Viewport::View viewport_view;
    teacup::widgets::List<Person>::View people_view;
    const teacup::layout::Areas areas = teacup::layout::resolve(showcase_tree(), teacup::Rect::from_size(m.screen_size));
    people_view.item_height(3).viewport_height(areas.get("people").height());
    const teacup::widgets::DrawingArea::View scene_view = drawing_view(m.raycaster, areas.get("drawing").size());

    const auto activate_menu = [&]
    {
        m.events.focus().set(m.menu.selected == 1 ? "drawing" : m.menu.selected == 2 ? "people" : "menu");
    };

    m.events.clear();
    m.events.add("menu", teacup::route_keys(m.menu, menu_view));
    m.events.add("tabs", teacup::route_keys(m.tabs, tabs_view));
    m.events.add("tabs", teacup::route_mouse(m.tabs, tabs_view.labels({ "gallery", "layout", "events" }), "tabs"));
    m.events.add("people", teacup::route_keys(m.people, people_view));
    m.events.add("people", teacup::route_mouse(m.people, people_view, "people"));
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

    if (const auto* key = teacup::msg_cast<teacup::KeyEvent>(message))
    {
        m.last_key = key->name();
        if (key->matches("ctrl+c"))
        {
            return teacup::next(m, teacup::quit());
        }

        const std::string was_focused = m.events.focus().focused();
        if (was_focused == "drawing" && steer(m.raycaster, *key, world))
        {
            return teacup::next(m);
        }
        const bool consumed = m.events.route(message, areas);

        if (key->code == teacup::Key::enter)
        {
            if (was_focused == "menu")
            {
                activate_menu();
            }
            else
            {
                m.menu.selected = was_focused == "drawing" ? 1 : was_focused == "people" ? 2 : 0;
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
                    .render(widgets::Progress::State{ (m.tick % 100) / 99.0 }, content.sub_absolute(areas.get("progress")));

                const bool drawing_focused = m.events.focus().is("drawing");
                const Point scroll = drawing_view(m.raycaster, areas.get("drawing").size())
                                         .visible_offset(m.drawing, areas.get("drawing").size());
                std::string title = drawing_focused ? " raycaster · wasd/arrows · q/e · m map " : " raycaster ";
                if (scroll != Point{})
                {
                    title += "@" + std::to_string(scroll.x) + "," + std::to_string(scroll.y) + " ";
                }
                input_block()
                    .border_style(Style{}.fg(drawing_focused ? palette::accent() : palette::subtle()))
                    .title(title)
                    .title_style(Style{}.fg(drawing_focused ? palette::accent() : palette::subtle()))
                    .render(
                        content.sub_absolute(areas.get("drawing-box")),
                        [&](Canvas drawing_area)
                        { drawing_view(m.raycaster, drawing_area.size()).render(m.drawing, drawing_area); });
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
    model.menu.items = { "overview", "raycaster", "people", "table", "progress" };
    model.tabs.selected = 0;
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