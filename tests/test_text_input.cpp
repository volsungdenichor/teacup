#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

TEST_CASE("text input edits UTF-8 text and moves its cursor")
{
    widgets::TextInput::State state{ "ab", 2 };
    widgets::TextInput::View view;

    CHECK(view.update(state, KeyEvent{ Key::left }));
    CHECK(state.cursor == 1);
    CHECK(view.update(state, KeyEvent{ Key::character, "é" }));
    CHECK(state.value == "aéb");
    CHECK(view.update(state, KeyEvent{ Key::space }));
    CHECK(state.value == "aé b");
    CHECK(view.update(state, KeyEvent{ Key::backspace }));
    CHECK(state.value == "aéb");
    CHECK(view.update(state, KeyEvent{ Key::del }));
    CHECK(state.value == "aé");
}

TEST_CASE("text input renders a horizontally clipped cursor")
{
    Surface s{ Size{ 5, 1 } };
    s.clear();
    Canvas c{ s };

    widgets::TextInput::State state{ "abcdef", 6 };
    widgets::TextInput::View view;
    view.render(state, c);

    CHECK(s.row_text(0) == "cdef ");
}

TEST_CASE("text input renders its current value")
{
    Surface s{ Size{ 12, 1 } };
    s.clear();
    Canvas c{ s };

    widgets::TextInput::State state{ "edited", 6 };
    widgets::TextInput::View view;
    view.render(state, c);

    CHECK(s.row_text(0) == "edited      ");
}
