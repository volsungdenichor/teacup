#include <catch2/catch_test_macros.hpp>

#include "teacup/canvas.hpp"
#include "teacup/widgets/block.hpp"

using namespace teacup;

TEST_CASE("canvas clips writes to its area")
{
    surface s{ size{ 6, 2 } };
    s.clear();
    canvas c{ s };

    c.text(point{ 0, 0 }, "abcdefghij");
    c.text(point{ -2, 1 }, "xy");

    CHECK(s.row_text(0) == "abcdef");
    CHECK(s.row_text(1) == "      ");
}

TEST_CASE("sub canvas translates and clips coordinates")
{
    surface s{ size{ 8, 3 } };
    s.clear();
    canvas c{ s };

    canvas inner = c.sub(rect{ 2, 1, 3, 1 });
    REQUIRE(inner.size() == size{ 3, 1 });

    inner.text(point{ 0, 0 }, "abcdef");

    CHECK(s.row_text(0) == "        ");
    CHECK(s.row_text(1) == "  abc   ");
}

TEST_CASE("block reserves border and padding")
{
    widgets::block b;
    b.border(border::rounded()).padding(1);
    CHECK(b.inner(rect{ 0, 0, 10, 6 }) == rect{ 2, 2, 6, 2 });

    surface s{ size{ 6, 3 } };
    s.clear();
    canvas c{ s };
    widgets::block{}.border(border::ascii()).render(c);

    CHECK(s.row_text(0) == "+----+");
    CHECK(s.row_text(1) == "|    |");
    CHECK(s.row_text(2) == "+----+");
}

TEST_CASE("block title is centered inside the top border")
{
    surface s{ size{ 9, 3 } };
    s.clear();
    canvas c{ s };
    widgets::block{}.border(border::ascii()).title("hi").title_align(align::center).render(c);

    CHECK(s.row_text(0) == "+--hi---+");
}

TEST_CASE("wide glyphs occupy two columns")
{
    surface s{ size{ 4, 1 } };
    s.clear();
    canvas c{ s };
    c.text(point{ 0, 0 }, "\xe6\x97\xa5x");  // U+65E5 then 'x'

    CHECK(s.at(point{0, 0}).width == 2);
    CHECK(s.at(point{1, 0}).width == 0);
    CHECK(s.at(point{2, 0}).text == "x");
}

