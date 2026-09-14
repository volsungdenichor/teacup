#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/list.hpp"

using namespace teacup;

TEST_CASE("list widget renders a selected item")
{
    surface s{ size{ 12, 3 } };
    s.clear();
    canvas c{ s };

    widgets::list items;
    items.items({ "alpha", "beta", "gamma" }).selected(1).render(c);

    CHECK(s.row_text(0) == "  alpha     ");
    CHECK(s.row_text(1) == "> beta      ");
    CHECK(s.row_text(2) == "  gamma     ");
}

TEST_CASE("list widget changes selection from key events")
{
    widgets::list items;
    items.items({ "alpha", "beta", "gamma" });

    CHECK(items.update(key_event{ key::down }));
    CHECK(items.selected() == 1);
    CHECK(items.update(key_event{ key::character, "k" }));
    CHECK(items.selected() == 0);
    CHECK(items.update(key_event{ key::end }));
    CHECK(items.selected() == 2);
}
