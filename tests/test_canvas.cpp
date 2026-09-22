#include <catch2/catch_test_macros.hpp>

#include "teacup/canvas.hpp"
#include "teacup/widgets/block.hpp"

using namespace teacup;

TEST_CASE("canvas clips writes to its area")
{
    Surface s{ Size{ 6, 2 } };
    s.clear();
    Canvas c{ s };

    c.text(Point{ 0, 0 }, "abcdefghij");
    c.text(Point{ -2, 1 }, "xy");

    CHECK(s.row_text(0) == "abcdef");
    CHECK(s.row_text(1) == "      ");
}

TEST_CASE("sub canvas translates and clips coordinates")
{
    Surface s{ Size{ 8, 3 } };
    s.clear();
    Canvas c{ s };

    Canvas inner = c.sub(Rect{ 2, 1, 3, 1 });
    REQUIRE(inner.size() == Size{ 3, 1 });

    inner.text(Point{ 0, 0 }, "abcdef");

    CHECK(s.row_text(0) == "        ");
    CHECK(s.row_text(1) == "  abc   ");
}

TEST_CASE("block reserves border and padding")
{
    widgets::Block b;
    b.border(Border::rounded()).padding(1);
    CHECK(b.inner(Rect{ 0, 0, 10, 6 }) == Rect{ 2, 2, 6, 2 });

    Surface s{ Size{ 6, 3 } };
    s.clear();
    Canvas c{ s };
    widgets::Block{}.border(Border::ascii()).render(c);

    CHECK(s.row_text(0) == "+----+");
    CHECK(s.row_text(1) == "|    |");
    CHECK(s.row_text(2) == "+----+");
}

TEST_CASE("block title is centered inside the top border")
{
    Surface s{ Size{ 9, 3 } };
    s.clear();
    Canvas c{ s };
    widgets::Block{}.border(Border::ascii()).title("hi").title_align(Align::center).render(c);

    CHECK(s.row_text(0) == "+--hi---+");
}

TEST_CASE("wide glyphs occupy two columns")
{
    Surface s{ Size{ 4, 1 } };
    s.clear();
    Canvas c{ s };
    c.text(Point{ 0, 0 }, "\xe6\x97\xa5x");  // U+65E5 then 'x'

    CHECK(s.at(Point{ 0, 0 }).width == 2);
    CHECK(s.at(Point{ 1, 0 }).width == 0);
    CHECK(s.at(Point{ 2, 0 }).text == "x");
}
