// Drives the REAL showcase update() with synthesized key/mouse events to
// prove keyboard and mouse reach the widgets. The showcase's main() is
// renamed away so this test links the translation unit directly.
#define main showcase_main
#include "../examples/showcase/main.cpp"
#undef main

#include <catch2/catch_test_macros.hpp>

using namespace teacup;

namespace
{

model fresh_model()
{
    model initial;
    initial.menu.items = { "overview", "text input", "players", "table", "progress" };
    initial.input.value = "type here";
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
    model m = fresh_model();
    m = update(std::move(m), message{ key_event{ key::none } }).model;
    CHECK(m.events.focus().focused() == "menu");
}

TEST_CASE("showcase update(): arrows navigate hub, enter activates players, arrows move it")
{
    model m = fresh_model();
    auto step = [&](key_event k) { m = update(std::move(m), message{ k }).model; };

    step(key_event{ key::none });
    INFO("after none: focused=" << m.events.focus().focused() << " menu.items=" << m.menu.items.size());
    step(key_event{ key::down });
    INFO("after down1: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    step(key_event{ key::down });
    INFO("after down2: focused=" << m.events.focus().focused() << " menu.sel=" << m.menu.selected);
    CHECK(m.menu.selected == 2);  // "players"

    step(key_event{ key::enter });
    INFO("after enter: focused=" << m.events.focus().focused());
    CHECK(m.events.focus().focused() == "players");

    step(key_event{ key::down });
    CHECK(m.players.selected == 1);
    step(key_event{ key::down });
    CHECK(m.players.selected == 2);
    step(key_event{ key::up });
    CHECK(m.players.selected == 1);

    step(key_event{ key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 2);    // hub mirrors where we came from
}

TEST_CASE("showcase update(): tab cycles focus")
{
    model m = fresh_model();
    auto step = [&](key_event k) { m = update(std::move(m), message{ k }).model; };

    step(key_event{ key::none });
    const std::string first = m.events.focus().focused();
    step(key_event{ key::tab });
    const std::string second = m.events.focus().focused();
    CHECK(first != second);
}

TEST_CASE("showcase update(): mouse click focuses and selects a player")
{
    model m = fresh_model();
    m = update(std::move(m), message{ key_event{ key::none } }).model;  // establish ring

    const layout::areas areas = layout::resolve(showcase_tree(), rect::from_size(m.screen_size));
    const rect players = areas.get("players");

    // Click the second player's first row (item_height 3 → y = players.y + 3).
    const point click{ players.x + 1, players.y + 3 };
    m = update(std::move(m),
               message{ mouse_event{ click, mouse_button::left, mouse_action::press } })
            .model;

    CHECK(m.events.focus().focused() == "players");
    CHECK(m.players.selected == 1);
}

TEST_CASE("showcase update(): text input receives keys when focused")
{
    model m = fresh_model();
    auto step = [&](key_event k) { m = update(std::move(m), message{ k }).model; };

    step(key_event{ key::none });
    step(key_event{ key::down });   // menu -> "text input"
    step(key_event{ key::enter });  // activate input
    REQUIRE(m.events.focus().focused() == "input");

    step(key_event{ key::character, "!" });
    CHECK(m.input.value == "type here!");
    step(key_event{ key::backspace });
    CHECK(m.input.value == "type here");
    step(key_event{ key::left });
    CHECK(m.input.cursor == m.input.value.size() - 1);

    step(key_event{ key::enter });  // back out to the hub
    CHECK(m.events.focus().focused() == "menu");
    CHECK(m.menu.selected == 1);
}
