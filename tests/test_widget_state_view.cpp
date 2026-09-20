#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/checkbox.hpp"
#include "teacup/widgets/list.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/tabs.hpp"
#include "teacup/widgets/text.hpp"
#include "teacup/widgets/text_input.hpp"
#include "teacup/widgets/viewport.hpp"

using namespace teacup;

namespace
{
struct menu_item
{
    std::string label;
};
}

TEST_CASE("list view updates application-owned list state")
{
    widgets::list<std::string>::state_type state{ { "one", "two" }, 0 };
    widgets::list<std::string>::view_type view;

    REQUIRE(view.update(state, key_event{ key::down }));
    CHECK(state.selected == 1);
}

TEST_CASE("generic list view renders widget-like items")
{
    surface s{ size{ 12, 2 } };
    s.clear();
    canvas c{ s };

    widgets::list<menu_item>::state_type state{ { { "first" }, { "second" } }, 1 };
    widgets::list<menu_item>::view_type view;
    view.item_renderer([](const menu_item& item, canvas row, const style& item_style, bool selected) {
        row.text(0, 0, (selected ? "[" : " ") + item.label + (selected ? "]" : " "), item_style);
    });
    view.render(state, c);

    CHECK(s.row_text(0) == " first      ");
    CHECK(s.row_text(1) == "[second]    ");
}

TEST_CASE("list view allows replacing the default string renderer")
{
    surface s{ size{ 10, 2 } };
    s.clear();
    canvas c{ s };

    widgets::list<std::string>::state_type state{ { "one", "two" }, 1 };
    widgets::list<std::string>::view_type view;
    view.item_renderer([](const std::string& item, canvas row, const style& item_style, bool selected) {
        row.text(0, 0, (selected ? "[" : " ") + item + (selected ? "]" : " "), item_style);
    });
    view.render(state, c);

    CHECK(s.row_text(0) == " one      ");
    CHECK(s.row_text(1) == "[two]     ");
}

TEST_CASE("text input view updates application-owned text state")
{
    widgets::text_input::state_type state{ "ab", 2 };
    widgets::text_input::view_type view;

    REQUIRE(view.update(state, key_event{ key::space }));
    CHECK(state.value == "ab ");
    CHECK(state.cursor == 3);
}

TEST_CASE("viewport view updates application-owned scroll state")
{
    widgets::viewport::state_type state{ { "one", "two", "three" }, 0 };
    widgets::viewport::view_type view;

    view.page_down(state, 2);
    CHECK(state.offset == 2);
}

TEST_CASE("tabs and spinner views update application-owned state")
{
    widgets::tabs::state_type tabs_state{};
    widgets::tabs::view_type tabs_view;
    tabs_view.labels({ "one", "two" });
    REQUIRE(tabs_view.update(tabs_state, key_event{ key::right }));
    CHECK(tabs_state.selected == 1);

    widgets::spinner::state_type spinner_state{};
    widgets::spinner::view_type spinner_view;
    spinner_view.frames({ ".", "o" });
    spinner_view.advance(spinner_state);
    CHECK(spinner_state.frame == 1);
}

TEST_CASE("widget views respond to mouse events")
{
    widgets::list<std::string>::state_type list_state{ { "one", "two", "three" }, 0 };
    widgets::list<std::string>::view_type list_view;
    CHECK(list_view.update(list_state, mouse_event{ point{ 2, 1 }, mouse_button::left, mouse_action::press }, rect{ 0, 0, 8, 3 }));
    CHECK(list_state.selected == 1);

    widgets::tabs::state_type tabs_state{};
    widgets::tabs::view_type tabs_view;
    tabs_view.labels({ "one", "two" });
    CHECK(tabs_view.update(tabs_state, mouse_event{ point{ 6, 0 }, mouse_button::left, mouse_action::press }, rect{ 0, 0, 12, 1 }));
    CHECK(tabs_state.selected == 1);

    widgets::viewport::state_type viewport_state{ { "one", "two", "three" }, 1 };
    widgets::viewport::view_type viewport_view;
    CHECK(viewport_view.update(viewport_state, mouse_event{ point{ 0, 0 }, mouse_button::wheel_up, mouse_action::press }, rect{ 0, 0, 8, 2 }));
    CHECK(viewport_state.offset == 0);

    widgets::text_input::state_type input_state{ "abc", 0 };
    widgets::text_input::view_type input_view;
    CHECK(input_view.update(input_state, mouse_event{ point{ 2, 0 }, mouse_button::left, mouse_action::press }, rect{ 0, 0, 8, 1 }));
    CHECK(input_state.cursor == 2);
}

TEST_CASE("widgets allow custom rendering into the allocated canvas")
{
    surface s{ size{ 12, 1 } };
    s.clear();
    canvas c{ s };

    widgets::text{ "ignored" }
        .renderer([](canvas target) { target.text(0, 0, "custom"); })
        .render(c);

    CHECK(s.row_text(0) == "custom      ");
}

TEST_CASE("checkbox view toggles from keyboard and mouse")
{
    widgets::checkbox::state_type state{ false };
    widgets::checkbox::view_type view;

    REQUIRE(view.update(state, key_event{ key::space }));
    CHECK(state.checked);
    REQUIRE(view.update(state, mouse_event{ point{ 0, 0 }, mouse_button::left, mouse_action::press }, rect{ 0, 0, 4, 1 }));
    CHECK_FALSE(state.checked);
}

TEST_CASE("generic list allocates multi-line item regions")
{
    surface s{ size{ 12, 4 } };
    s.clear();
    widgets::list<menu_item>::state_type state{ { { "first" }, { "second" } }, 0 };
    widgets::list<menu_item>::view_type view;
    view.item_height(2).item_renderer([](const menu_item& item, canvas row, const style& style, bool) {
        row.text(0, 0, item.label, style);
        row.text(0, 1, "detail", style);
    });
    view.render(state, canvas{ s });

    CHECK(s.row_text(0) == "first       ");
    CHECK(s.row_text(1) == "detail      ");
    CHECK(s.row_text(2) == "second      ");
    CHECK(s.row_text(3) == "detail      ");
}

TEST_CASE("generic list scrolls multi-line item regions")
{
    surface s{ size{ 12, 4 } };
    s.clear();
    widgets::list<menu_item>::state_type state{ { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::list<menu_item>::view_type view;
    view.item_height(2).item_renderer([](const menu_item& item, canvas row, const style& style, bool) {
        row.text(0, 0, item.label, style);
        row.text(0, 1, "detail", style);
    });

    view.page_down(state, 4);
    CHECK(state.offset == 1);
    view.render(state, canvas{ s });
    CHECK(s.row_text(0) == "second      ");
    CHECK(s.row_text(2) == "third       ");
}

TEST_CASE("generic list keeps the selected item visible")
{
    widgets::list<menu_item>::state_type state{ { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::list<menu_item>::view_type view;
    view.item_height(2);

    view.update(state, key_event{ key::end });
    view.ensure_visible(state, 4);

    CHECK(state.selected == 2);
    CHECK(state.offset == 1);
}

TEST_CASE("generic list scrolls enough to reveal the last item")
{
    widgets::list<menu_item>::state_type state{
        { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::list<menu_item>::view_type view;
    view.item_height(2).item_renderer([](const menu_item& item, canvas row, const style& style, bool) {
        row.text(0, 0, item.label, style);
        row.text(0, 1, "detail", style);
    });

    view.update(state, key_event{ key::end });
    view.ensure_visible(state, 5);
    surface surface_value{ size{ 12, 5 } };
    surface_value.clear();
    view.render(state, canvas{ surface_value });

    CHECK(state.offset == 1);
    CHECK(surface_value.row_text(0) == "second      ");
    CHECK(surface_value.row_text(2) == "third       ");
}
