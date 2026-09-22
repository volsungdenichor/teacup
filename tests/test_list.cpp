#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/list.hpp"

using namespace teacup;

TEST_CASE("list widget renders a selected item")
{
    Surface s{ Size{ 12, 3 } };
    s.clear();
    Canvas c{ s };

    widgets::List<std::string>::State state{ { "alpha", "beta", "gamma" }, 1 };
    widgets::List<std::string>::View view;
    view.render(state, c);

    CHECK(s.row_text(0) == "  alpha     ");
    CHECK(s.row_text(1) == "> beta      ");
    CHECK(s.row_text(2) == "  gamma     ");
}

TEST_CASE("list widget changes selection from key events")
{
    widgets::List<std::string>::State state{ { "alpha", "beta", "gamma" }, 0 };
    widgets::List<std::string>::View view;

    CHECK(view.update(state, KeyEvent{ Key::down }));
    CHECK(state.selected == 1);
    CHECK(view.update(state, KeyEvent{ Key::character, "k" }));
    CHECK(state.selected == 0);
    CHECK(view.update(state, KeyEvent{ Key::end }));
    CHECK(state.selected == 2);
}
