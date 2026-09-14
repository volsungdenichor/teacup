#include <catch2/catch_test_macros.hpp>

#include "teacup/event.hpp"
#include "teacup/surface.hpp"
#include "teacup/widgets/text_input.hpp"

using namespace teacup;

TEST_CASE("text input edits UTF-8 text and moves its cursor")
{
    widgets::text_input input;
    input.value("ab");

    CHECK(input.update(key_event{ key::left }));
    CHECK(input.cursor() == 1);
    CHECK(input.update(key_event{ key::character, "é" }));
    CHECK(input.value() == "aéb");
    CHECK(input.update(key_event{ key::space }));
    CHECK(input.value() == "aé b");
    CHECK(input.update(key_event{ key::backspace }));
    CHECK(input.value() == "aéb");
    CHECK(input.update(key_event{ key::del }));
    CHECK(input.value() == "aé");
}

TEST_CASE("text input renders a horizontally clipped cursor")
{
    surface s{ size{ 5, 1 } };
    s.clear();
    canvas c{ s };

    widgets::text_input input;
    input.value("abcdef").render(c);

    CHECK(s.row_text(0) == "cdef ");
}

TEST_CASE("text input renders its current value")
{
    surface s{ size{ 12, 1 } };
    s.clear();
    canvas c{ s };

    widgets::text_input input;
    input.value("edited").cursor(6).render(c);

    CHECK(s.row_text(0) == "edited      ");
}