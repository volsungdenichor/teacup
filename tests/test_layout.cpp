#include <catch2/catch_test_macros.hpp>

#include "teacup/layout.hpp"

using namespace teacup;

TEST_CASE("fixed and flex constraints share the available width")
{
    const auto parts = split_horizontal(
        rect{ 0, 0, 20, 3 }, { constraint::fixed(5), constraint::flex(), constraint::fixed(5) });

    REQUIRE(parts.size() == 3);
    CHECK(parts[0] == rect{ 0, 0, 5, 3 });
    CHECK(parts[1] == rect{ 5, 0, 10, 3 });
    CHECK(parts[2] == rect{ 15, 0, 5, 3 });
}

TEST_CASE("percent constraints are relative to the available space")
{
    const auto parts = split_vertical(
        rect{ 0, 0, 10, 10 }, { constraint::percent(30), constraint::flex() });

    REQUIRE(parts.size() == 2);
    CHECK(parts[0].height() == 3);
    CHECK(parts[1].height() == 7);
}

TEST_CASE("gaps are subtracted before distribution")
{
    const auto parts = split_horizontal(
        rect{ 0, 0, 11, 1 }, { constraint::flex(), constraint::flex() }, 1);

    REQUIRE(parts.size() == 2);
    CHECK(parts[0] == rect{ 0, 0, 5, 1 });
    CHECK(parts[1] == rect{ 6, 0, 5, 1 });
}

TEST_CASE("weighted flex constraints split proportionally")
{
    const auto parts = split_horizontal(
        rect{ 0, 0, 9, 1 }, { constraint::flex(1), constraint::flex(2) });

    REQUIRE(parts.size() == 2);
    CHECK(parts[0].width() == 3);
    CHECK(parts[1].width() == 6);
}

TEST_CASE("centered clamps to the parent area")
{
    CHECK(centered(rect{ 0, 0, 10, 4 }, 4, 2) == rect{ 3, 1, 4, 2 });
    CHECK(centered(rect{ 0, 0, 4, 2 }, 10, 10) == rect{ 0, 0, 4, 2 });
}
