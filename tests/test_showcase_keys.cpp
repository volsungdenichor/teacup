
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
    initial.menu.items = { "overview", "text input", "players", "table", "progress" };
    initial.input.value = GlyphString::from_utf8("type here");
    initial.input.cursor = initial.input.value.size();
    initial.players.items = {
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

TEST_CASE("showcase update(): arrows navigate hub, enter activates players, arrows move it")
{
    Model m = fresh_model();
    auto step = [&](KeyEvent k) { m = update(std::move(m), Message{ k }).model; };

    step(KeyEvent{ Key::none });
    INFO("after none: focused=" << m.events.focus().focused() << " menu.items=" << m.menu.items.size());
    step(KeyEvent{ Key::down });
    INFO("after down1: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    step(KeyEvent{ Key::down });
    INFO("after down2: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    CHECK(m.menu.selected == 2);  // "players"

    step(KeyEvent{ Key::enter });
    INFO("after enter: focused=" << m.events.focus().focused());
    CHECK(m.events.focus().focused() == "players");

    step(KeyEvent{ Key::down });
    CHECK(m.players.selected == 1);
    step(KeyEvent{ Key::down });
    CHECK(m.players.selected == 2);
    step(KeyEvent{ Key::up });
    CHECK(m.players.selected == 1);

    step(KeyEvent{ Key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 2);    // hub mirrors where we came from
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
    const Rect players = areas.get("players");
    m.players.offset = 1;  // the area fits a single 3-row item, so scroll to show the second one

    const auto click = Point{ players.left() + 1, players.top() + 1 };
    m = update(std::move(m),
               Message{ MouseEvent{ click, MouseButton::left, MouseAction::press } })
            .model;

    CHECK(m.events.focus().focused() == "players");
    CHECK(m.players.selected == 1);
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
