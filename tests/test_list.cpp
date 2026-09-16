#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/list.hpp"

using namespace teacup;

TEST_CASE("list widget renders a selected item")
{
    surface s{ size{ 12, 3 } };
    s.clear();
    canvas c{ s };

    widgets::list_state state{ { "alpha", "beta", "gamma" }, 1 };
    widgets::list_view view;
    view.render(state, c);

    CHECK(s.row_text(0) == "  alpha     ");
    CHECK(s.row_text(1) == "> beta      ");
    CHECK(s.row_text(2) == "  gamma     ");
}

TEST_CASE("list widget changes selection from key events")
{
    widgets::list_state state{ { "alpha", "beta", "gamma" }, 0 };
    widgets::list_view view;

    CHECK(view.update(state, key_event{ key::down }));
    CHECK(state.selected == 1);
    CHECK(view.update(state, key_event{ key::character, "k" }));
    CHECK(state.selected == 0);
    CHECK(view.update(state, key_event{ key::end }));
    CHECK(state.selected == 2);
}
