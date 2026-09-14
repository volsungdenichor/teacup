#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/viewport.hpp"

using namespace teacup;

TEST_CASE("viewport renders a clipped window and scrolls by pages")
{
    surface s{ size{ 10, 3 } };
    s.clear();
    canvas c{ s };

    widgets::viewport view;
    view.lines({ "zero", "one", "two", "three", "four" }).render(c);

    CHECK(s.row_text(0) == "zero      ");
    CHECK(s.row_text(1) == "one       ");
    CHECK(s.row_text(2) == "two       ");

    view.page_down(3).render(c);
    CHECK(view.offset() == 3);
    CHECK(s.row_text(0) == "two       ");
    CHECK(s.row_text(1) == "three     ");
    CHECK(s.row_text(2) == "four      ");

    view.offset(99).render(c);
    CHECK(view.offset() == 5);
    CHECK(s.row_text(0) == "two       ");
    CHECK(s.row_text(1) == "three     ");
    CHECK(s.row_text(2) == "four      ");
}