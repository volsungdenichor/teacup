#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/tabs.hpp"

using namespace teacup;

TEST_CASE("spinner advances through custom frames")
{
    Surface s{ Size{ 4, 1 } };
    Canvas c{ s };
    widgets::Spinner::State state{};
    widgets::Spinner::View view;
    view.frames({ ".", "o", "O" });
    state.frame = 1;
    view.render(state, c);
    CHECK(s.row_text(0) == "o   ");
    view.advance(state);
    view.render(state, c);
    CHECK(s.row_text(0) == "O   ");
}

TEST_CASE("tabs navigate and render labels")
{
    Surface s{ Size{ 16, 1 } };
    Canvas c{ s };
    widgets::Tabs::State state{};
    widgets::Tabs::View view;
    view.labels({ "one", "two", "three" });
    state.selected = 1;
    view.render(state, c);
    CHECK(s.row_text(0) == " one [two] three");
    CHECK(view.update(state, KeyEvent{ Key::right }));
    CHECK(state.selected == 2);
    CHECK(view.update(state, KeyEvent{ Key::right }));
    CHECK(state.selected == 0);
}
