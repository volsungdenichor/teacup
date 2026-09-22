#include <catch2/catch_test_macros.hpp>

#include "teacup/color.hpp"
#include "teacup/style.hpp"

using namespace teacup;

TEST_CASE("hex parsing supports short and long forms")
{
    CHECK(Color::hex("#7d56f4") == Color::rgb(0x7d, 0x56, 0xf4));
    CHECK(Color::hex("f0a") == Color::rgb(0xff, 0x00, 0xaa));
    CHECK_FALSE(Color::hex("nope").is_set());
}

TEST_CASE("colors degrade to the terminal profile")
{
    const Color c = Color::rgb(0xff, 0x00, 0x00);
    CHECK(c.adapt(ColorProfile::truecolor) == c);
    CHECK(c.adapt(ColorProfile::ansi256).type() == Color::Kind::indexed);
    CHECK(c.adapt(ColorProfile::ansi16) == Color::indexed(1));
    CHECK_FALSE(c.adapt(ColorProfile::no_color).is_set());
}

TEST_CASE("mix interpolates between two colors")
{
    const Color mid = Color::mix(Color::rgb(0, 0, 0), Color::rgb(100, 200, 50), 0.5);
    CHECK(mid == Color::rgb(50, 100, 25));
}

TEST_CASE("sgr sequence encodes attributes and colors")
{
    const Style s = Style{}.fg(Color::rgb(1, 2, 3)).bold();
    CHECK(sgr_sequence(s, ColorProfile::truecolor) == "\x1b[0;1;38;2;1;2;3m");
    CHECK(sgr_sequence(s, ColorProfile::no_color) == "\x1b[0;1m");
}

TEST_CASE("styles merge with the right-hand side winning")
{
    const Style base = Style{}.fg(Color::indexed(1)).bold();
    const Style over = Style{}.fg(Color::indexed(2)).italic();
    const Style result = base | over;

    CHECK(result.foreground() == Color::indexed(2));
    CHECK(has(result.attributes(), Attr::bold));
    CHECK(has(result.attributes(), Attr::italic));
}
