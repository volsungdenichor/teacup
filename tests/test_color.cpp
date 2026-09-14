#include <catch2/catch_test_macros.hpp>

#include "teacup/color.hpp"
#include "teacup/style.hpp"

using namespace teacup;

TEST_CASE("hex parsing supports short and long forms")
{
    CHECK(color::hex("#7d56f4") == color::rgb(0x7d, 0x56, 0xf4));
    CHECK(color::hex("f0a") == color::rgb(0xff, 0x00, 0xaa));
    CHECK_FALSE(color::hex("nope").is_set());
}

TEST_CASE("colors degrade to the terminal profile")
{
    const color c = color::rgb(0xff, 0x00, 0x00);
    CHECK(c.adapt(color_profile::truecolor) == c);
    CHECK(c.adapt(color_profile::ansi256).type() == color::kind::indexed);
    CHECK(c.adapt(color_profile::ansi16) == color::indexed(1));
    CHECK_FALSE(c.adapt(color_profile::no_color).is_set());
}

TEST_CASE("mix interpolates between two colors")
{
    const color mid = color::mix(color::rgb(0, 0, 0), color::rgb(100, 200, 50), 0.5);
    CHECK(mid == color::rgb(50, 100, 25));
}

TEST_CASE("sgr sequence encodes attributes and colors")
{
    const style s = style{}.fg(color::rgb(1, 2, 3)).bold();
    CHECK(sgr_sequence(s, color_profile::truecolor) == "\x1b[0;1;38;2;1;2;3m");
    CHECK(sgr_sequence(s, color_profile::no_color) == "\x1b[0;1m");
}

TEST_CASE("styles merge with the right-hand side winning")
{
    const style base = style{}.fg(color::indexed(1)).bold();
    const style over = style{}.fg(color::indexed(2)).italic();
    const style result = base | over;

    CHECK(result.foreground() == color::indexed(2));
    CHECK(has(result.attributes(), attr::bold));
    CHECK(has(result.attributes(), attr::italic));
}
