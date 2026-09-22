#include <catch2/catch_test_macros.hpp>

#include "teacup/layout.hpp"

using namespace teacup;

TEST_CASE("fixed and flex constraints share the available width")
{
    const auto parts
        = split_horizontal(Rect{ 0, 0, 20, 3 }, { Constraint::fixed(5), Constraint::flex(), Constraint::fixed(5) });

    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == Rect{ 0, 0, 5, 3 });
    CHECK(parts[1] == Rect{ 5, 0, 10, 3 });
    CHECK(parts[2] == Rect{ 15, 0, 5, 3 });
}

TEST_CASE("percent constraints are relative to the available space")
{
    const auto parts = split_vertical(Rect{ 0, 0, 10, 10 }, { Constraint::percent(30), Constraint::flex() });

    REQUIRE(parts.size() == 2);
    CHECK(parts[0].height() == 3);
    CHECK(parts[1].height() == 7);
}

TEST_CASE("gaps are subtracted before distribution")
{
    const auto parts = split_horizontal(Rect{ 0, 0, 11, 1 }, { Constraint::flex(), Constraint::flex() }, 1);

    REQUIRE(parts.size() == 2);
    CHECK(parts[0] == Rect{ 0, 0, 5, 1 });
    CHECK(parts[1] == Rect{ 6, 0, 5, 1 });
}

TEST_CASE("weighted flex constraints split proportionally")
{
    const auto parts = split_horizontal(Rect{ 0, 0, 9, 1 }, { Constraint::flex(1), Constraint::flex(2) });

    REQUIRE(parts.size() == 2);
    CHECK(parts[0].width() == 3);
    CHECK(parts[1].width() == 6);
}

TEST_CASE("centered clamps to the parent area")
{
    CHECK(centered(Rect{ 0, 0, 10, 4 }, 4, 2) == Rect{ 3, 1, 4, 2 });
    CHECK(centered(Rect{ 0, 0, 4, 2 }, 10, 10) == Rect{ 0, 0, 4, 2 });
}
