#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/list.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/tabs.hpp"
#include "teacup/widgets/text_input.hpp"
#include "teacup/widgets/viewport.hpp"

using namespace teacup;

TEST_CASE("list view updates application-owned list state")
{
    widgets::list_state state{ { "one", "two" }, 0 };
    widgets::list_view view;

    REQUIRE(view.update(state, key_event{ key::down }));
    CHECK(state.selected == 1);
}

TEST_CASE("text input view updates application-owned text state")
{
    widgets::text_input_state state{ "ab", 2 };
    widgets::text_input_view view;

    REQUIRE(view.update(state, key_event{ key::space }));
    CHECK(state.value == "ab ");
    CHECK(state.cursor == 3);
}

TEST_CASE("viewport view updates application-owned scroll state")
{
    widgets::viewport_state state{ { "one", "two", "three" }, 0 };
    widgets::viewport_view view;

    view.page_down(state, 2);
    CHECK(state.offset == 2);
}

TEST_CASE("tabs and spinner views update application-owned state")
{
    widgets::tabs_state tabs_state{};
    widgets::tabs_view tabs_view;
    tabs_view.labels({ "one", "two" });
    REQUIRE(tabs_view.update(tabs_state, key_event{ key::tab }));
    CHECK(tabs_state.selected == 1);

    widgets::spinner_state spinner_state{};
    widgets::spinner_view spinner_view;
    spinner_view.frames({ ".", "o" });
    spinner_view.advance(spinner_state);
    CHECK(spinner_state.frame == 1);
}
