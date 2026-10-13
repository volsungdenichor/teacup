
#define main showcase_main
#include "../examples/showcase/main.cpp"
#undef main

#include <catch2/catch_test_macros.hpp>

using namespace teacup;

namespace
{

Model fresh_model()
{
    Model initial;
    initial.menu.items = { "overview", "raycaster", "people", "table", "progress" };
    initial.people.items = {
        { "Name: McEnroe", Country::usa, true },
        { "Name: Agassi", Country::usa, true },
        { "Name: Navratilova", Country::czech_republic, true },
    };
    return initial;
}

}  // namespace

TEST_CASE("showcase update(): focus starts on the menu hub")
{
    Model m = fresh_model();
    m = update(std::move(m), Message{ KeyEvent{ Key::none } }).model;
    CHECK(m.events.focus().focused() == "menu");
}

TEST_CASE("showcase update(): arrows navigate hub, enter activates people, arrows move it")
{
    Model m = fresh_model();
    auto step = [&](KeyEvent k) { m = update(std::move(m), Message{ k }).model; };

    step(KeyEvent{ Key::none });
    INFO("after none: focused=" << m.events.focus().focused() << " menu.items=" << m.menu.items.size());
    step(KeyEvent{ Key::down });
    INFO("after down1: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    step(KeyEvent{ Key::down });
    INFO("after down2: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    CHECK(m.menu.selected == 2);  // "people"

    step(KeyEvent{ Key::enter });
    INFO("after enter: focused=" << m.events.focus().focused());
    CHECK(m.events.focus().focused() == "people");

    step(KeyEvent{ Key::down });
    CHECK(m.people.selected == 1);
    step(KeyEvent{ Key::down });
    CHECK(m.people.selected == 2);
    step(KeyEvent{ Key::up });
    CHECK(m.people.selected == 1);

    step(KeyEvent{ Key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 2);  // hub mirrors where we came from
}

TEST_CASE("showcase update(): tab cycles focus")
{
    Model m = fresh_model();
    auto step = [&](KeyEvent k) { m = update(std::move(m), Message{ k }).model; };

    step(KeyEvent{ Key::none });
    const std::string first = m.events.focus().focused();
    step(KeyEvent{ Key::tab });
    const std::string second = m.events.focus().focused();
    CHECK(first != second);
}

TEST_CASE("showcase update(): mouse click focuses and selects a player")
{
    Model m = fresh_model();
    m = update(std::move(m), Message{ KeyEvent{ Key::none } }).model;  // establish ring

    const layout::Areas areas = layout::resolve(showcase_tree(), Rect::from_size(m.screen_size));
    const Rect people = areas.get("people");
    m.people.offset = 1;  // the area fits a single 3-row item, so scroll to show the second one

    const auto click = Point{ people.left() + 1, people.top() + 1 };
    m = update(std::move(m), Message{ MouseEvent{ click, MouseButton::left, MouseAction::press } }).model;

    CHECK(m.events.focus().focused() == "people");
    CHECK(m.people.selected == 1);
}

TEST_CASE("showcase update(): raycaster moves and turns when focused")
{
    Model m = fresh_model();
    auto step = [&](KeyEvent k) { m = update(std::move(m), Message{ k }).model; };

    step(KeyEvent{ Key::none });
    step(KeyEvent{ Key::down });   // menu -> "raycaster"
    step(KeyEvent{ Key::enter });  // activate the drawing area
    REQUIRE(m.events.focus().focused() == "drawing");

    const Raycaster start = m.raycaster;
    step(KeyEvent{ Key::character, "w" });
    CHECK(m.raycaster.x > start.x);
    CHECK(m.raycaster.y > start.y);
    step(KeyEvent{ Key::right });
    CHECK(m.raycaster.angle > start.angle);
    step(KeyEvent{ Key::character, "m" });
    CHECK(m.raycaster.show_map);
    CHECK(m.menu.selected == 1);  // movement keys never reach the menu

    step(KeyEvent{ Key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 1);
}

TEST_CASE("raycaster: rays measure perpendicular distance to the first wall")
{
    // From (1.5, 1.5) looking east, the first wall is the '#' at column 7.
    const auto world = World{};
    const RayHit east = cast_ray(1.5, 1.5, 1.0, 0.0, world);
    CHECK(east.wall == '#');
    CHECK(east.vertical_face);
    CHECK(std::abs(east.distance - 5.5) < 1e-9);

    const RayHit north = cast_ray(1.5, 1.5, 0.0, -1.0, world);
    CHECK_FALSE(north.vertical_face);
    CHECK(std::abs(north.distance - 0.5) < 1e-9);

    const RayHit pillar = cast_ray(1.5, 2.5, 1.0, 0.0, world);
    CHECK(pillar.wall == '2');
    CHECK(std::abs(pillar.distance - 1.5) < 1e-9);
}

TEST_CASE("raycaster: walls block movement but the player slides along them")
{
    Raycaster r;
    r.x = 1.5;
    r.y = 1.5;
    r.angle = -M_PI / 4.0;  // north-east, into the top wall
    for (int i = 0; i < 20; ++i)
    {
        r.move(0.25, 0.0);
    }
    CHECK(r.y >= 1.0 + r.world.player_radius);
    CHECK(r.x > 3.0);  // slid east along the wall
    CHECK(r.x < 7.0 - r.world.player_radius);
}

TEST_CASE("raycaster: render fills the view with sky, walls and floor")
{
    Raycaster r;
    r.x = 1.5;
    r.y = 1.5;
    r.angle = 0.0;
    r.show_map = false;
    Surface frame{ Size{ 20, 10 } };
    draw_raycaster(r, Canvas{ frame });

    const Cell& top = frame.at(Point{ 10, 0 });
    const Cell& middle = frame.at(Point{ 10, 5 });
    const Cell& bottom = frame.at(Point{ 10, 9 });
    CHECK(middle.glyph.utf8() == "▀");
    CHECK(top.style != middle.style);
    CHECK(bottom.style != middle.style);
    CHECK(top.style != bottom.style);
}

TEST_CASE("showcase layout: blocks and event areas agree across screen sizes")
{
    for (const Size size : { Size{ 80, 24 }, Size{ 120, 40 }, Size{ 32, 12 }, Size{ 4, 3 } })
    {
        Model m = fresh_model();
        m = update(std::move(m), Message{ ResizeEvent{ size } }).model;
        CHECK(m.screen_size == size);
        const layout::Areas areas = layout::resolve(showcase_tree(), Rect::from_size(size));
        Surface frame{ size };
        view(m, Canvas{ frame });

        for (const std::string id : { "title", "spinner", "tabs", "menu", "people", "progress", "drawing-box", "drawing" })
        {
            const std::string* parent = areas.parent(id);
            REQUIRE(parent != nullptr);
            const Rect outer = areas.get(*parent);
            const widgets::Block block = *parent == "drawing-box" ? input_block() : panel_block();
            const Rect inner = block.inner(outer).intersect(outer);
            CHECK(areas.get(id).intersect(inner) == areas.get(id));
        }

        const Rect drawing = areas.get("drawing");
        if (!drawing.empty())
        {
            CHECK(frame.at(drawing.origin()).glyph.utf8() == "▀");
            m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::left, MouseAction::press } }).model;
            CHECK(m.events.focus().focused() == "drawing");
        }
    }
}

TEST_CASE("showcase update(): drawing area scrolls the 3D view when the window is too small")
{
    Model m = fresh_model();
    m = update(std::move(m), Message{ ResizeEvent{ Size{ 80, 18 } } }).model;
    m = update(std::move(m), Message{ KeyEvent{ Key::none } }).model;
    const layout::Areas areas = layout::resolve(showcase_tree(), Rect::from_size(m.screen_size));
    const Rect drawing = areas.get("drawing");
    REQUIRE_FALSE(drawing.empty());
    REQUIRE(drawing.height() < min_render_size.height);

    m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::wheel_down } }).model;
    CHECK(m.drawing.offset == Point{ 0, 1 });

    m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::left, MouseAction::press } }).model;
    REQUIRE(m.events.focus().focused() == "drawing");
    m = update(std::move(m), Message{ KeyEvent{ Key::character, "j" } }).model;
    CHECK(m.drawing.offset == Point{ 0, 2 });
    m = update(std::move(m), Message{ KeyEvent{ Key::page_down } }).model;
    CHECK(m.drawing.offset.y == (std::min)(2 + drawing.height(), min_render_size.height - drawing.height()));
    CHECK(m.viewport.offset == 0);
    m = update(std::move(m), Message{ KeyEvent{ Key::home } }).model;
    CHECK(m.drawing.offset == Point{ 0, 0 });
}

TEST_CASE("showcase update(): a window big enough for the 3D view does not scroll")
{
    Model m = fresh_model();
    m = update(std::move(m), Message{ ResizeEvent{ Size{ 120, 40 } } }).model;
    m = update(std::move(m), Message{ KeyEvent{ Key::none } }).model;
    const Rect drawing = layout::resolve(showcase_tree(), Rect::from_size(m.screen_size)).get("drawing");
    m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::wheel_down } }).model;
    CHECK(m.drawing.offset == Point{ 0, 0 });
}
