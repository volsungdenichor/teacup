#include <catch2/catch_test_macros.hpp>

#include "teacup/focus.hpp"
#include "teacup/widgets/list.hpp"
#include "teacup/widgets/tabs.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

namespace
{

layout::Node two_pane_tree()
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
    FocusRing focus{ { "tabs", "menu", "input" } };

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
    widgets::List<std::string>::State menu{ { "a", "b" }, 0 };
    widgets::TextInput::State input{ GlyphString::from_utf8("x"), 1 };

    Router events;
    events.add("menu", route_keys(menu, widgets::List<std::string>::View{}));
    events.add("input", route_keys(input, widgets::TextInput::View{}));
    events.focus_order({ "menu", "input" });
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(Message{ KeyEvent{ Key::down } }, layout));
    CHECK(menu.selected == 1);  // menu is focused, list ate the key
    CHECK(input.value.to_utf8() == "x");

    events.focus().set("input");
    REQUIRE(events.route(Message{ KeyEvent{ Key ::left } }, layout));
    CHECK(menu.selected == 1);  // menu no longer sees keys
    CHECK(input.cursor == 0);   // text_input moved the cursor
}

TEST_CASE("router cycles focus with tab and shift+tab")
{
    widgets::TextInput::State input{};

    Router events;
    events.add("input", route_keys(input, widgets::TextInput::View{}));
    events.focus_order({ "tabs", "menu", "input" });
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(Message{ KeyEvent{ Key::tab } }, layout));
    CHECK(events.focus().focused() == "menu");
    REQUIRE(events.route(Message{ KeyEvent{ Key::tab, "", Modifier::shift } }, layout));
    CHECK(events.focus().focused() == "tabs");
}

TEST_CASE("mouse press focuses the hit area and routes the event")
{
    widgets::List<std::string>::State menu{ { "a", "b", "c" }, 0 };

    Router events;
    events.add("left", route_mouse(menu, widgets::List<std::string>::View{}, "left"));
    events.focus_order({ "left" });  // the ring only lists bound widgets
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    // A click on the "left" leaf focuses and routes to the bound list.
    const MouseEvent click{ Point{ 3, 2 }, MouseButton::left, MouseAction::press };
    REQUIRE(events.route(Message{ click }, layout));
    CHECK(events.focus().focused() == "left");
    CHECK(menu.selected == 2);

    // A click in an area with no bound handler keeps the current focus.
    const MouseEvent click_right{ Point{ 15, 9 }, MouseButton::left, MouseAction::press };
    CHECK_FALSE(events.route(Message{ click_right }, layout));
    CHECK(events.focus().focused() == "left");
}

TEST_CASE("mouse focus walks up to the nearest focusable ancestor")
{
    widgets::List<std::string>::State menu{ { "a", "b" }, 0 };

    Router events;
    // Bound under the container id; the layout names the inner leaf.
    events.add("menu", route_mouse(menu, widgets::List<std::string>::View{}, "items"));
    events.focus_order({ "menu" });
    const Areas layout
        = layout::resolve(layout::named("menu", layout::inset(1, layout::leaf("items"))), Rect{ 0, 0, 10, 5 });

    // "items" has no handler; its named ancestor "menu" does.
    events.route(Message{ MouseEvent{ Point{ 2, 2 }, MouseButton::left, MouseAction::press } }, layout);
    CHECK(events.focus().focused() == "menu");
}

TEST_CASE("focus ring with no ids stays unfocused")
{
    FocusRing focus;

    CHECK(focus.focused().empty());
    focus.advance();
    CHECK(focus.focused().empty());
    CHECK_FALSE(focus.is("anything"));
}

TEST_CASE("tab always cycles focus and is never dispatched to widgets")
{
    widgets::Tabs::State tabs;
    widgets::List<std::string>::State menu{ { "a", "b" }, 0 };

    Router events;
    events.add("tabs", route_keys(tabs, widgets::Tabs::View{}.labels({ "x", "y", "z" })));
    events.add("menu", route_keys(menu, widgets::List<std::string>::View{}));
    events.focus_order({ "tabs", "menu" });
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    // tabs would accept tab to cycle labels, but the router keeps it.
    REQUIRE(events.route(Message{ KeyEvent{ Key::tab } }, layout));
    CHECK(events.focus().focused() == "menu");
    CHECK(tabs.selected == 0);  // unchanged: the key never reached the widget

    REQUIRE(events.route(Message{ KeyEvent{ Key::tab } }, layout));
    CHECK(events.focus().focused() == "tabs");
}

TEST_CASE("a hub widget can hand focus off and take it back")
{
    widgets::List<std::string>::State menu{ { "overview", "editor" }, 0 };
    widgets::TextInput::State input{};

    Router events;
    // Menu arrows move only the selection; focus changes on enter.
    events.add("menu", route_keys(menu, widgets::List<std::string>::View{}));
    events.add("editor", route_keys(input, widgets::TextInput::View{}));
    events.focus_order({ "menu", "editor" });
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(Message{ KeyEvent{ Key::down } }, layout));
    CHECK(menu.selected == 1);
    CHECK(events.focus().focused() == "menu");  // selection moved, focus did not

    // App policy: enter activates the selection.
    if (menu.selected == 1)
        events.focus().set("editor");
    REQUIRE(events.route(Message{ KeyEvent{ Key::character, "a" } }, layout));
    CHECK(input.value.to_utf8() == "a");

    // App policy: enter backs out to the hub.
    events.focus().set("menu");
    REQUIRE(events.route(Message{ KeyEvent{ Key::up } }, layout));
    CHECK(menu.selected == 0);
}

TEST_CASE("a focused list navigates while the hub selection mirrors focus")
{
    // Mirrors the showcase: a menu hub + a players list, focus authoritative.
    widgets::List<std::string>::State menu{ { "overview", "editor", "players" }, 0 };
    widgets::List<std::string>::State players{ { "a", "b", "c", "d" }, 0 };

    Router events;
    events.add("menu", route_keys(menu, widgets::List<std::string>::View{}));
    events.add("players", route_keys(players, widgets::List<std::string>::View{}));
    const Areas layout = layout::resolve(
        layout::column({
            layout::fixed(4, layout::focusable("menu")),
            layout::flex(layout::focusable("players")),
        }),
        Rect{ 0, 0, 20, 12 });

    auto activate_menu = [&] {
        events.focus().set(menu.selected == 1 ? "editor" : menu.selected == 2 ? "players" : "menu");
    };

    // Hub: arrow to "players", enter activates.
    events.route(Message{ KeyEvent{ Key::none } }, layout);  // adopt focus order
    events.route(Message{ KeyEvent{ Key::down } }, layout);
    events.route(Message{ KeyEvent{ Key::down } }, layout);
    CHECK(menu.selected == 2);
    activate_menu();
    CHECK(events.focus().focused() == "players");

    // Now arrows must reach the players list.
    events.route(Message{ KeyEvent{ Key::down } }, layout);
    events.route(Message{ KeyEvent{ Key::down } }, layout);
    CHECK(players.selected == 2);
    events.route(Message{ KeyEvent{ Key::up } }, layout);
    CHECK(players.selected == 1);

    // Enter backs out; the hub selection mirrors the widget we came from.
    menu.selected = events.focus().focused() == "players" ? 2 : 0;
    events.focus().set("menu");
    CHECK(menu.selected == 2);
}

TEST_CASE("router adopts the layout-declared focus order")
{
    widgets::TextInput::State input{ GlyphString::from_utf8("x"), 1 };

    Router events;
    events.add("bottom", route_keys(input, widgets::TextInput::View{}));
    // No focus_order() call: the tree declares the tab stops, adopted lazily.
    const Areas layout = layout::resolve(
        layout::row({
            layout::fixed(10, layout::focusable("left")),
            layout::flex(layout::focusable("bottom")),
        }),
        Rect{ 0, 0, 20, 10 });

    REQUIRE(events.route(Message{ KeyEvent{ Key::tab } }, layout));  // adopts + advances
    CHECK(events.focus().focused() == "bottom");
    REQUIRE(events.route(Message{ KeyEvent{ Key::left } }, layout));
    CHECK(input.cursor == 0);
}

TEST_CASE("disabled entries are skipped and cannot take focus")
{
    WidgetTraits traits;
    traits.enabled([](const std::string& id) { return id != "menu"; });

    FocusRing focus{ { "tabs", "menu", "input" } };
    focus.traits(traits);

    CHECK(focus.focused() == "tabs");
    focus.advance();
    CHECK(focus.focused() == "input");  // menu skipped
    focus.advance(-1);
    CHECK(focus.focused() == "tabs");  // and skipped backwards

    CHECK_FALSE(focus.set("menu"));  // disabled ids reject focus
    CHECK(focus.focused() == "tabs");
}

TEST_CASE("when nothing is focusable the ring reports no focus")
{
    WidgetTraits traits;
    traits.enabled([](const std::string&) { return false; });

    FocusRing focus{ { "a", "b" } };
    focus.traits(traits);

    CHECK(focus.focused().empty());
    focus.advance();
    CHECK(focus.focused().empty());
    CHECK_FALSE(focus.set("a"));
}

TEST_CASE("focusable-only filtering keeps events but drops tab stops")
{
    WidgetTraits traits;
    traits.focusable([](const std::string& id) { return id != "status"; });

    FocusRing focus{ { "input", "status" } };
    focus.traits(traits);

    focus.advance();
    CHECK(focus.focused() == "input");  // status is never a tab stop
    CHECK_FALSE(focus.set("status"));
}

TEST_CASE("disabled widgets receive no events through route helpers")
{
    widgets::List<std::string>::State menu{ { "a", "b" }, 0 };

    Router events;
    events.add("left", route_keys(menu, widgets::List<std::string>::View{}, false));
    events.add("left", route_mouse(menu, widgets::List<std::string>::View{}, "left", false));
    const Areas layout = layout::resolve(two_pane_tree(), Rect{ 0, 0, 20, 10 });

    CHECK_FALSE(events.route(Message{ KeyEvent{ Key::down } }, layout));
    CHECK_FALSE(events.route(Message{ MouseEvent{ Point{ 3, 1 }, MouseButton::left, MouseAction::press } }, layout));
    CHECK(menu.selected == 0);  // untouched
}
