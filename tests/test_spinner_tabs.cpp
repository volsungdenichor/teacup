#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/tabs.hpp"

using namespace teacup;

TEST_CASE("spinner advances through custom frames")
{
    surface s{ size{ 4, 1 } };
    canvas c{ s };
    widgets::spinner_state state{};
    widgets::spinner_view view;
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
    surface s{ size{ 16, 1 } };
    canvas c{ s };
    widgets::tabs_state state{};
    widgets::tabs_view view;
    view.labels({ "one", "two", "three" });
    state.selected = 1;
    view.render(state, c);
    CHECK(s.row_text(0) == " one [two] three");
    CHECK(view.update(state, key_event{ key::tab }));
    CHECK(state.selected == 2);
    CHECK(view.update(state, key_event{ key::right }));
    CHECK(state.selected == 0);
}
