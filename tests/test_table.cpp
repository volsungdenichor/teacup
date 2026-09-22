#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/table.hpp"

using namespace teacup;

TEST_CASE("table widget renders headers and rows")
{
    Surface s{ Size{ 20, 4 } };
    s.clear();
    Canvas c{ s };

    widgets::Table t;
    t.headers({ "name", "score" })
        .rows({ { "alpha", "10" }, { "beta", "2" } })
        .render(c);

    CHECK(s.row_text(0) == "name  score         ");
    CHECK(s.row_text(1) == "alpha 10            ");
    CHECK(s.row_text(2) == "beta  2             ");
    CHECK(s.row_text(3) == "                    ");
}
