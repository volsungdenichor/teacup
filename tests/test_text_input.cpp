#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

TEST_CASE("text input edits UTF-8 text and moves its cursor")
{
    widgets::TextInput::State state{ GlyphString::from_utf8("ab"), 2 };
    widgets::TextInput::View view;

    CHECK(view.update(state, KeyEvent{ Key::left }));
    CHECK(state.cursor == 1);
    CHECK(view.update(state, KeyEvent{ Key::character, "é" }));
    CHECK(state.value.to_utf8() == "aéb");
    CHECK(view.update(state, KeyEvent{ Key::space }));
    CHECK(state.value.to_utf8() == "aé b");
    CHECK(view.update(state, KeyEvent{ Key::backspace }));
    CHECK(state.value.to_utf8() == "aéb");
    CHECK(view.update(state, KeyEvent{ Key::del }));
    CHECK(state.value.to_utf8() == "aé");
}

TEST_CASE("text input treats grapheme clusters as single characters")
{
    widgets::TextInput::State state{ GlyphString::from_utf8("e"), 1 };
    widgets::TextInput::View view;

    CHECK(view.update(state, KeyEvent{ Key::character, "\xcc\x81" }));  // COMBINING ACUTE joins the 'e'
    CHECK(state.value.size() == 1);
    CHECK(state.cursor == 1);
    CHECK(view.update(state, KeyEvent{ Key::backspace }));
    CHECK(state.value.empty());
}

TEST_CASE("text input renders a horizontally clipped cursor")
{
    Surface s{ Size{ 5, 1 } };
    s.clear();
    Canvas c{ s };

    widgets::TextInput::State state{ GlyphString::from_utf8("abcdef"), 6 };
    widgets::TextInput::View view;
    view.render(state, c);

    CHECK(s.row_text(0) == "cdef ");
}

TEST_CASE("text input renders its current value")
{
    Surface s{ Size{ 12, 1 } };
    s.clear();
    Canvas c{ s };

    widgets::TextInput::State state{ GlyphString::from_utf8("edited"), 6 };
    widgets::TextInput::View view;
    view.render(state, c);

    CHECK(s.row_text(0) == "edited      ");
}
