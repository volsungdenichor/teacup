#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/tabs.hpp"

using namespace teacup;

TEST_CASE("spinner advances through custom frames")
{
    surface s{ size{ 4, 1 } };
    canvas c{ s };
    widgets::spinner value;
    value.frames({ ".", "o", "O" }).frame(1).render(c);
    CHECK(s.row_text(0) == "o   ");
    value.advance().render(c);
    CHECK(s.row_text(0) == "O   ");
}

TEST_CASE("tabs navigate and render labels")
{
    surface s{ size{ 16, 1 } };
    canvas c{ s };
    widgets::tabs value;
    value.labels({ "one", "two", "three" }).selected(1).render(c);
    CHECK(s.row_text(0) == " one [two] three");
    CHECK(value.update(key_event{ key::tab }));
    CHECK(value.selected() == 2);
    CHECK(value.update(key_event{ key::right }));
    CHECK(value.selected() == 0);
}