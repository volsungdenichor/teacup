
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
    initial.menu.items = { "overview", "text input", "people", "table", "progress" };
    initial.input.value = GlyphString::from_utf8("type here");
    initial.input.cursor = initial.input.value.size();
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

TEST_CASE("showcase update(): text input receives keys when focused")
{
    Model m = fresh_model();
    auto step = [&](KeyEvent k) { m = update(std::move(m), Message{ k }).model; };

    step(KeyEvent{ Key::none });
    step(KeyEvent{ Key::down });   // menu -> "text input"
    step(KeyEvent{ Key::enter });  // activate input
    REQUIRE(m.events.focus().focused() == "input");

    step(KeyEvent{ Key::character, "!" });
    CHECK(m.input.value.to_utf8() == "type here!");
    step(KeyEvent{ Key::backspace });
    CHECK(m.input.value.to_utf8() == "type here");
    step(KeyEvent{ Key::left });
    CHECK(m.input.cursor == m.input.value.size() - 1);

    step(KeyEvent{ Key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 1);
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

        for (const std::string id :
            { "title",
              "spinner",
              "tabs",
              "menu",
              "people",
              "progress",
              "table",
              "input-box",
              "input",
              "drawing-box",
              "drawing" })
        {
            const std::string* parent = areas.parent(id);
            REQUIRE(parent != nullptr);
            const Rect outer = areas.get(*parent);
            const widgets::Block block = *parent == "input-box" || *parent == "drawing-box" ? input_block() : panel_block();
            const Rect inner = block.inner(outer).intersect(outer);
            CHECK(areas.get(id).intersect(inner) == areas.get(id));
        }

        const Rect input = areas.get("input");
        if (!input.empty())
        {
            CHECK(input.height() == 1);
            CHECK(frame.at(input.origin()).glyph.utf8() == "t");
            m = update(std::move(m), Message{ MouseEvent{
                input.origin(), MouseButton::left, MouseAction::press } }).model;
            CHECK(m.events.focus().focused() == "input");
            const Point preview = input.origin() + Point{ 0, 1 };
            REQUIRE(areas.hit(preview) != nullptr);
            CHECK(*areas.hit(preview) != "input");
        }
    }
}

TEST_CASE("showcase update(): drawing area scrolls with the wheel and keys once focused")
{
    Model m = fresh_model();
    m = update(std::move(m), Message{ ResizeEvent{ Size{ 120, 40 } } }).model;
    m = update(std::move(m), Message{ KeyEvent{ Key::none } }).model;
    const layout::Areas areas = layout::resolve(showcase_tree(), Rect::from_size(m.screen_size));
    const Rect drawing = areas.get("drawing");
    REQUIRE_FALSE(drawing.empty());

    m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::wheel_down } }).model;
    CHECK(m.drawing.offset == Point{ 0, 1 });

    m = update(std::move(m), Message{ MouseEvent{ drawing.origin(), MouseButton::left, MouseAction::press } }).model;
    REQUIRE(m.events.focus().focused() == "drawing");
    m = update(std::move(m), Message{ KeyEvent{ Key::right } }).model;
    CHECK(m.drawing.offset == Point{ 1, 1 });
    m = update(std::move(m), Message{ KeyEvent{ Key::page_down } }).model;
    CHECK(m.drawing.offset.y == (std::min)(1 + drawing.height(), drawing_content_size.height - drawing.height()));
    CHECK(m.viewport.offset == 0);

    m = update(std::move(m), Message{ KeyEvent{ Key::home } }).model;
    CHECK(m.drawing.offset == Point{ 0, 0 });
    Surface frame{ m.screen_size };
    view(m, Canvas{ frame });
    CHECK(frame.at(drawing.origin()).glyph.utf8() == "+");
    CHECK(frame.at(drawing.origin() + Point{ 1, 0 }).glyph.utf8() == "0");
}
