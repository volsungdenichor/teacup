#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

TEST_CASE("text input edits UTF-8 text and moves its cursor")
{
    widgets::text_input_state state{ "ab", 2 };
    widgets::text_input_view view;

    CHECK(view.update(state, key_event{ key::left }));
    CHECK(state.cursor == 1);
    CHECK(view.update(state, key_event{ key::character, "é" }));
    CHECK(state.value == "aéb");
    CHECK(view.update(state, key_event{ key::space }));
    CHECK(state.value == "aé b");
    CHECK(view.update(state, key_event{ key::backspace }));
    CHECK(state.value == "aéb");
    CHECK(view.update(state, key_event{ key::del }));
    CHECK(state.value == "aé");
}

TEST_CASE("text input renders a horizontally clipped cursor")
{
    surface s{ size{ 5, 1 } };
    s.clear();
    canvas c{ s };

    widgets::text_input_state state{ "abcdef", 6 };
    widgets::text_input_view view;
    view.render(state, c);

    CHECK(s.row_text(0) == "cdef ");
}

TEST_CASE("text input renders its current value")
{
    surface s{ size{ 12, 1 } };
    s.clear();
    canvas c{ s };

    widgets::text_input_state state{ "edited", 6 };
    widgets::text_input_view view;
    view.render(state, c);

    CHECK(s.row_text(0) == "edited      ");
}
