#include <catch2/catch_test_macros.hpp>

#include "teacup/focus.hpp"
#include "teacup/widgets/list.hpp"
#include "teacup/widgets/tabs.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

namespace
{

layout::node two_pane_tree()
{
    using namespace layout;
    return row({
        fixed(10, leaf("left")),
        flex(column({
            fixed(3, leaf("top")),
            flex(leaf("bottom")),
        })),
    });
}

}  // namespace

TEST_CASE("focus ring cycles through its order")
{
    focus_ring focus{ { "tabs", "menu", "input" } };

    CHECK(focus.focused() == "tabs");
    focus.advance();
    CHECK(focus.focused() == "menu");
    focus.advance(2);
    CHECK(focus.focused() == "tabs");
    focus.advance(-1);
    CHECK(focus.focused() == "input");

    CHECK(focus.set("input"));
    CHECK(focus.focused() == "input");
    CHECK_FALSE(focus.set("nowhere"));
    CHECK(focus.focused() == "input");
}

TEST_CASE("router delivers keys only to the focused widget")
{
    widgets::list<std::string>::state_type menu{ { "a", "b" }, 0 };
    widgets::text_input::state_type input{ "x", 1 };

    router events;
    events.add("menu", route_keys(menu, widgets::list<std::string>::view_type{}));
    events.add("input", route_keys(input, widgets::text_input::view_type{}));
    events.focus_order({ "menu", "input" });
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(message{ key_event{ key::down } }, layout));
    CHECK(menu.selected == 1);   // menu is focused, list ate the key
    CHECK(input.value == "x");

    events.focus().set("input");
    REQUIRE(events.route(message{ key_event{ key::left } }, layout));
    CHECK(menu.selected == 1);   // menu no longer sees keys
    CHECK(input.cursor == 0);    // text_input moved the cursor
}

TEST_CASE("router cycles focus with tab and shift+tab")
{
    widgets::text_input::state_type input{ "", 0 };

    router events;
    events.add("input", route_keys(input, widgets::text_input::view_type{}));
    events.focus_order({ "tabs", "menu", "input" });
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(message{ key_event{ key::tab } }, layout));
    CHECK(events.focus().focused() == "menu");
    REQUIRE(events.route(
        message{ key_event{ key::tab, "", modifier::shift } }, layout));
    CHECK(events.focus().focused() == "tabs");
}

TEST_CASE("mouse press focuses the hit area and routes the event")
{
    widgets::list<std::string>::state_type menu{ { "a", "b", "c" }, 0 };

    router events;
    events.add("left", route_mouse(menu, widgets::list<std::string>::view_type{}, "left"));
    events.focus_order({ "left" });  // the ring only lists bound widgets
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    // A click on the "left" leaf focuses and routes to the bound list.
    const mouse_event click{ point{ 3, 2 }, mouse_button::left, mouse_action::press };
    REQUIRE(events.route(message{ click }, layout));
    CHECK(events.focus().focused() == "left");
    CHECK(menu.selected == 2);

    // A click in an area with no bound handler keeps the current focus.
    const mouse_event click_right{ point{ 15, 9 }, mouse_button::left, mouse_action::press };
    CHECK_FALSE(events.route(message{ click_right }, layout));
    CHECK(events.focus().focused() == "left");
}

TEST_CASE("mouse focus walks up to the nearest focusable ancestor")
{
    widgets::list<std::string>::state_type menu{ { "a", "b" }, 0 };

    router events;
    // Bound under the container id; the layout names the inner leaf.
    events.add("menu", route_mouse(menu, widgets::list<std::string>::view_type{}, "items"));
    events.focus_order({ "menu" });
    const areas layout = layout::resolve(
        layout::named("menu", layout::inset(1, layout::leaf("items"))), rect{ 0, 0, 10, 5 });

    // "items" has no handler; its named ancestor "menu" does.
    events.route(message{ mouse_event{ point{ 2, 2 }, mouse_button::left, mouse_action::press } }, layout);
    CHECK(events.focus().focused() == "menu");
}

TEST_CASE("focus ring with no ids stays unfocused")
{
    focus_ring focus;

    CHECK(focus.focused().empty());
    focus.advance();
    CHECK(focus.focused().empty());
    CHECK_FALSE(focus.is("anything"));
}

TEST_CASE("tab always cycles focus and is never dispatched to widgets")
{
    widgets::tabs::state_type tabs;
    widgets::list<std::string>::state_type menu{ { "a", "b" }, 0 };

    router events;
    events.add("tabs", route_keys(tabs, widgets::tabs::view_type{}.labels({ "x", "y", "z" })));
    events.add("menu", route_keys(menu, widgets::list<std::string>::view_type{}));
    events.focus_order({ "tabs", "menu" });
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    // tabs would accept tab to cycle labels, but the router keeps it.
    REQUIRE(events.route(message{ key_event{ key::tab } }, layout));
    CHECK(events.focus().focused() == "menu");
    CHECK(tabs.selected == 0);  // unchanged: the key never reached the widget

    REQUIRE(events.route(message{ key_event{ key::tab } }, layout));
    CHECK(events.focus().focused() == "tabs");
}

TEST_CASE("a hub widget can hand focus off and take it back")
{
    widgets::list<std::string>::state_type menu{ { "overview", "editor" }, 0 };
    widgets::text_input::state_type input{ "", 0 };

    router events;
    // Menu arrows move only the selection; focus changes on enter.
    events.add("menu", route_keys(menu, widgets::list<std::string>::view_type{}));
    events.add("editor", route_keys(input, widgets::text_input::view_type{}));
    events.focus_order({ "menu", "editor" });
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(message{ key_event{ key::down } }, layout));
    CHECK(menu.selected == 1);
    CHECK(events.focus().focused() == "menu");  // selection moved, focus did not

    // App policy: enter activates the selection.
    if (menu.selected == 1)
        events.focus().set("editor");
    REQUIRE(events.route(message{ key_event{ key::character, "a" } }, layout));
    CHECK(input.value == "a");

    // App policy: enter backs out to the hub.
    events.focus().set("menu");
    REQUIRE(events.route(message{ key_event{ key::up } }, layout));
    CHECK(menu.selected == 0);
}

TEST_CASE("a focused list navigates while the hub selection mirrors focus")
{
    // Mirrors the showcase: a menu hub + a players list, focus authoritative.
    widgets::list<std::string>::state_type menu{ { "overview", "editor", "players" }, 0 };
    widgets::list<std::string>::state_type players{ { "a", "b", "c", "d" }, 0 };

    router events;
    events.add("menu", route_keys(menu, widgets::list<std::string>::view_type{}));
    events.add("players", route_keys(players, widgets::list<std::string>::view_type{}));
    const areas layout = layout::resolve(
        layout::column({
            layout::fixed(4, layout::focusable("menu")),
            layout::flex(layout::focusable("players")),
        }),
        rect{ 0, 0, 20, 12 });

    auto activate_menu = [&] {
        events.focus().set(menu.selected == 1 ? "editor" : menu.selected == 2 ? "players" : "menu");
    };

    // Hub: arrow to "players", enter activates.
    events.route(message{ key_event{ key::none } }, layout);  // adopt focus order
    events.route(message{ key_event{ key::down } }, layout);
    events.route(message{ key_event{ key::down } }, layout);
    CHECK(menu.selected == 2);
    activate_menu();
    CHECK(events.focus().focused() == "players");

    // Now arrows must reach the players list.
    events.route(message{ key_event{ key::down } }, layout);
    events.route(message{ key_event{ key::down } }, layout);
    CHECK(players.selected == 2);
    events.route(message{ key_event{ key::up } }, layout);
    CHECK(players.selected == 1);

    // Enter backs out; the hub selection mirrors the widget we came from.
    menu.selected = events.focus().focused() == "players" ? 2 : 0;
    events.focus().set("menu");
    CHECK(menu.selected == 2);
}

TEST_CASE("router adopts the layout-declared focus order")
{
    widgets::text_input::state_type input{ "x", 1 };

    router events;
    events.add("bottom", route_keys(input, widgets::text_input::view_type{}));
    // No focus_order() call: the tree declares the tab stops, adopted lazily.
    const areas layout = layout::resolve(
        layout::row({
            layout::fixed(10, layout::focusable("left")),
            layout::flex(layout::focusable("bottom")),
        }),
        rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(message{ key_event{ key::tab } }, layout));  // adopts + advances
    CHECK(events.focus().focused() == "bottom");
    REQUIRE(events.route(message{ key_event{ key::left } }, layout));
    CHECK(input.cursor == 0);
}

TEST_CASE("disabled entries are skipped and cannot take focus")
{
    widget_traits traits;
    traits.enabled([](const std::string& id) { return id != "menu"; });

    focus_ring focus{ { "tabs", "menu", "input" } };
    focus.traits(traits);

    CHECK(focus.focused() == "tabs");
    focus.advance();
    CHECK(focus.focused() == "input");  // menu skipped
    focus.advance(-1);
    CHECK(focus.focused() == "tabs");   // and skipped backwards

    CHECK_FALSE(focus.set("menu"));     // disabled ids reject focus
    CHECK(focus.focused() == "tabs");
}

TEST_CASE("when nothing is focusable the ring reports no focus")
{
    widget_traits traits;
    traits.enabled([](const std::string&) { return false; });

    focus_ring focus{ { "a", "b" } };
    focus.traits(traits);

    CHECK(focus.focused().empty());
    focus.advance();
    CHECK(focus.focused().empty());
    CHECK_FALSE(focus.set("a"));
}

TEST_CASE("focusable-only filtering keeps events but drops tab stops")
{
    widget_traits traits;
    traits.focusable([](const std::string& id) { return id != "status"; });

    focus_ring focus{ { "input", "status" } };
    focus.traits(traits);

    focus.advance();
    CHECK(focus.focused() == "input");  // status is never a tab stop
    CHECK_FALSE(focus.set("status"));
}

TEST_CASE("disabled widgets receive no events through route helpers")
{
    widgets::list<std::string>::state_type menu{ { "a", "b" }, 0 };

    router events;
    events.add("left", route_keys(menu, widgets::list<std::string>::view_type{}, false));
    events.add("left", route_mouse(menu, widgets::list<std::string>::view_type{}, "left", false));
    const areas layout = layout::resolve(two_pane_tree(), rect{ 0, 0, 20, 10 });

    CHECK_FALSE(events.route(message{ key_event{ key::down } }, layout));
    CHECK_FALSE(events.route(
        message{ mouse_event{ point{ 3, 1 }, mouse_button::left, mouse_action::press } }, layout));
    CHECK(menu.selected == 0);  // untouched
}
