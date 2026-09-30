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
struct MenuItem
{
    std::string label;
};
}  // namespace

TEST_CASE("list view updates application-owned list state")
{
    widgets::List<std::string>::State state{ { "one", "two" }, 0 };
    widgets::List<std::string>::View view;

    REQUIRE(view.update(state, KeyEvent{ Key::down }));
    CHECK(state.selected == 1);
}

TEST_CASE("generic list view renders widget-like items")
{
    Surface s{ Size{ 12, 2 } };
    s.clear();
    Canvas c{ s };

    widgets::List<MenuItem>::State state{ { { "first" }, { "second" } }, 1 };
    widgets::List<MenuItem>::View view;
    view.item_renderer(
        [](const MenuItem& item, Canvas row, const Style& item_style, const widgets::ListItemState& state)
        { row.text(Point{ 0, 0 }, (state.selected ? "[" : " ") + item.label + (state.selected ? "]" : " "), item_style); });
    view.render(state, c);

    CHECK(s.row_text(0) == " first      ");
    CHECK(s.row_text(1) == "[second]    ");
}

TEST_CASE("list view allows replacing the default string renderer")
{
    Surface s{ Size{ 10, 2 } };
    s.clear();
    Canvas c{ s };

    widgets::List<std::string>::State state{ { "one", "two" }, 1 };
    widgets::List<std::string>::View view;
    view.item_renderer(
        [](const std::string& item, Canvas row, const Style& item_style, const widgets::ListItemState& state)
        { row.text(Point{ 0, 0 }, (state.selected ? "[" : " ") + item + (state.selected ? "]" : " "), item_style); });
    view.render(state, c);

    CHECK(s.row_text(0) == " one      ");
    CHECK(s.row_text(1) == "[two]     ");
}

TEST_CASE("text input view updates application-owned text state")
{
    widgets::TextInput::State state{ "ab", 2 };
    widgets::TextInput::View view;

    REQUIRE(view.update(state, KeyEvent{ Key::space }));
    CHECK(state.value == "ab ");
    CHECK(state.cursor == 3);
}

TEST_CASE("viewport view updates application-owned scroll state")
{
    widgets::Viewport::State state{ { "one", "two", "three" }, 0 };
    widgets::Viewport::View view;

    view.page_down(state, 2);
    CHECK(state.offset == 2);
}

TEST_CASE("tabs and spinner views update application-owned state")
{
    widgets::Tabs::State tabs_state{};
    widgets::Tabs::View tabs_view;
    tabs_view.labels({ "one", "two" });
    REQUIRE(tabs_view.update(tabs_state, KeyEvent{ Key::right }));
    CHECK(tabs_state.selected == 1);

    widgets::Spinner::State spinner_state{};
    widgets::Spinner::View spinner_view;
    spinner_view.frames({ ".", "o" });
    spinner_view.advance(spinner_state);
    CHECK(spinner_state.frame == 1);
}

TEST_CASE("widget views respond to mouse events")
{
    widgets::List<std::string>::State list_state{ { "one", "two", "three" }, 0 };
    widgets::List<std::string>::View list_view;
    CHECK(list_view.update(
        list_state, MouseEvent{ Point{ 2, 1 }, MouseButton::left, MouseAction::press }, Rect{ 0, 0, 8, 3 }));
    CHECK(list_state.selected == 1);

    widgets::Tabs::State tabs_state{};
    widgets::Tabs::View tabs_view;
    tabs_view.labels({ "one", "two" });
    CHECK(tabs_view.update(
        tabs_state, MouseEvent{ Point{ 6, 0 }, MouseButton::left, MouseAction::press }, Rect{ 0, 0, 12, 1 }));
    CHECK(tabs_state.selected == 1);

    widgets::Viewport::State viewport_state{ { "one", "two", "three" }, 1 };
    widgets::Viewport::View viewport_view;
    CHECK(viewport_view.update(
        viewport_state, MouseEvent{ Point{ 0, 0 }, MouseButton::wheel_up, MouseAction::press }, Rect{ 0, 0, 8, 2 }));
    CHECK(viewport_state.offset == 0);

    widgets::TextInput::State input_state{ "abc", 0 };
    widgets::TextInput::View input_view;
    CHECK(input_view.update(
        input_state, MouseEvent{ Point{ 2, 0 }, MouseButton::left, MouseAction::press }, Rect{ 0, 0, 8, 1 }));
    CHECK(input_state.cursor == 2);
}

TEST_CASE("widgets allow custom rendering into the allocated canvas")
{
    Surface s{ Size{ 12, 1 } };
    s.clear();
    Canvas c{ s };

    widgets::Text{ "ignored" }.renderer([](Canvas target) { target.text(Point{ 0, 0 }, "custom"); }).render(c);

    CHECK(s.row_text(0) == "custom      ");
}

TEST_CASE("checkbox view toggles from keyboard and mouse")
{
    widgets::Checkbox::State state{ false };
    widgets::Checkbox::View view;

    REQUIRE(view.update(state, KeyEvent{ Key::space }));
    CHECK(state.checked);
    REQUIRE(view.update(state, MouseEvent{ Point{ 0, 0 }, MouseButton::left, MouseAction::press }, Rect{ 0, 0, 4, 1 }));
    CHECK_FALSE(state.checked);
}

TEST_CASE("generic list allocates multi-line item regions")
{
    Surface s{ Size{ 12, 4 } };
    s.clear();
    widgets::List<MenuItem>::State state{ { { "first" }, { "second" } }, 0 };
    widgets::List<MenuItem>::View view;
    view.item_height(2).item_renderer(
        [](const MenuItem& item, Canvas row, const Style& style, const widgets::ListItemState&)
        {
            row.text(Point{ 0, 0 }, item.label, style);
            row.text(Point{ 0, 1 }, "detail", style);
        });
    view.render(state, Canvas{ s });

    CHECK(s.row_text(0) == "first       ");
    CHECK(s.row_text(1) == "detail      ");
    CHECK(s.row_text(2) == "second      ");
    CHECK(s.row_text(3) == "detail      ");
}

TEST_CASE("generic list scrolls multi-line item regions")
{
    Surface s{ Size{ 12, 4 } };
    s.clear();
    widgets::List<MenuItem>::State state{ { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::List<MenuItem>::View view;
    view.item_height(2).item_renderer(
        [](const MenuItem& item, Canvas row, const Style& style, const widgets::ListItemState&)
        {
            row.text(Point{ 0, 0 }, item.label, style);
            row.text(Point{ 0, 1 }, "detail", style);
        });

    view.page_down(state, 4);
    CHECK(state.offset == 1);
    view.render(state, Canvas{ s });
    CHECK(s.row_text(0) == "second      ");
    CHECK(s.row_text(2) == "third       ");
}

TEST_CASE("generic list keeps the selected item visible")
{
    widgets::List<MenuItem>::State state{ { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::List<MenuItem>::View view;
    view.item_height(2);

    view.update(state, KeyEvent{ Key::end });
    view.ensure_visible(state, 4);

    CHECK(state.selected == 2);
    CHECK(state.offset == 1);
}

TEST_CASE("generic list scrolls enough to reveal the last item")
{
    widgets::List<MenuItem>::State state{ { { "first" }, { "second" }, { "third" } }, 0 };
    widgets::List<MenuItem>::View view;
    view.item_height(2).item_renderer(
        [](const MenuItem& item, Canvas row, const Style& style, const widgets::ListItemState&)
        {
            row.text(Point{ 0, 0 }, item.label, style);
            row.text(Point{ 0, 1 }, "detail", style);
        });

    view.update(state, KeyEvent{ Key::end });
    view.ensure_visible(state, 5);
    Surface surface_value{ Size{ 12, 5 } };
    surface_value.clear();
    view.render(state, Canvas{ surface_value });

    CHECK(state.offset == 1);
    CHECK(surface_value.row_text(0) == "second      ");
    CHECK(surface_value.row_text(2) == "third       ");
}
