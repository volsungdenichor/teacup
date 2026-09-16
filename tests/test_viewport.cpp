#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/viewport.hpp"

using namespace teacup;

TEST_CASE("viewport renders a clipped window and scrolls by pages")
{
    surface s{ size{ 10, 3 } };
    s.clear();
    canvas c{ s };

    widgets::viewport_state state{ { "zero", "one", "two", "three", "four" }, 0 };
    widgets::viewport_view view;
    view.render(state, c);

    CHECK(s.row_text(0) == "zero      ");
    CHECK(s.row_text(1) == "one       ");
    CHECK(s.row_text(2) == "two       ");

    view.page_down(state, 3);
    view.render(state, c);
    CHECK(state.offset == 3);
    CHECK(s.row_text(0) == "two       ");
    CHECK(s.row_text(1) == "three     ");
    CHECK(s.row_text(2) == "four      ");

    state.offset = 99;
    view.render(state, c);
    CHECK(state.offset == 99);
    CHECK(s.row_text(0) == "two       ");
    CHECK(s.row_text(1) == "three     ");
    CHECK(s.row_text(2) == "four      ");
}
