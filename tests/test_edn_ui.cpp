#include <catch2/catch_test_macros.hpp>

#include "teacup/edn_ui.hpp"
#include "teacup/surface.hpp"

using namespace teacup;

namespace
{

Surface render(std::string_view text, Size size)
{
    Surface s{ size };
    s.clear();
    edn_ui::parse(text).render(Canvas{ s });
    return s;
}

}  // namespace

TEST_CASE("edn text widget renders content with alignment")
{
    const Surface s = render(R"((text "ab" :align :center))", Size{ 6, 1 });
    CHECK(s.row_text(0) == "  ab  ");
}

TEST_CASE("edn string shorthand builds a text widget")
{
    const Surface s = render(R"("hi")", Size{ 4, 1 });
    CHECK(s.row_text(0) == "hi  ");
}

TEST_CASE("edn text style sets color and attributes")
{
    const Surface s = render(R"((text "x" :style {:fg "#ff0000" :bold true}))", Size{ 1, 1 });
    const Style style = s.at(Point{ 0, 0 }).style;
    CHECK(style.foreground() == Color::rgb(255, 0, 0));
    CHECK(has(style.attributes(), Attr::bold));
}

TEST_CASE("edn block draws border, title and child")
{
    const Surface s = render(R"((block :border :rounded :title "t" (text "x")))", Size{ 5, 3 });
    CHECK(s.row_text(0) == "╭t──╮");
    CHECK(s.row_text(1) == "│x  │");
    CHECK(s.row_text(2) == "╰───╯");
}

TEST_CASE("edn column splits by size hints")
{
    const Surface s = render(R"((column (text "a" :size 1) (text "b")))", Size{ 1, 3 });
    CHECK(s.row_text(0) == "a");
    CHECK(s.row_text(1) == "b");
}

TEST_CASE("edn row places children side by side")
{
    const Surface s = render(R"((row (text "a" :size 2) (text "b")))", Size{ 4, 1 });
    CHECK(s.row_text(0) == "a b ");
}

TEST_CASE("edn centered positions its child")
{
    const Surface s = render(R"((centered 1 1 (text "x")))", Size{ 3, 3 });
    CHECK(s.row_text(1) == " x ");
}

TEST_CASE("edn progress fills by ratio")
{
    const Surface s = render(R"((progress 0.5 :glyphs ["#" "-"]))", Size{ 4, 1 });
    CHECK(s.row_text(0) == "##--");
}

TEST_CASE("edn checkbox renders checked state")
{
    CHECK(render(R"((checkbox true))", Size{ 3, 1 }).row_text(0) == "[+]");
    CHECK(render(R"((checkbox false :glyphs ["[x]" "[o]"]))", Size{ 3, 1 }).row_text(0) == "[o]");
}

TEST_CASE("edn spinner shows the requested frame")
{
    const Surface s = render(R"((spinner 1 :frames ["a" "b" "c"]))", Size{ 1, 1 });
    CHECK(s.row_text(0) == "b");
}

TEST_CASE("edn tabs highlight the selected label")
{
    const Surface s = render(R"((tabs ["aa" "bb"] :selected 1))", Size{ 8, 1 });
    CHECK(s.row_text(0) == " aa [bb]");
}

TEST_CASE("edn list marks the selected item")
{
    const Surface s = render(R"((list ["one" "two"] :selected 1))", Size{ 5, 2 });
    CHECK(s.row_text(0) == "  one");
    CHECK(s.row_text(1) == "> two");
}

TEST_CASE("edn table lays out headers and rows")
{
    const Surface s = render(R"((table [["1" "2"]] :headers ["a" "b"]))", Size{ 3, 2 });
    CHECK(s.row_text(0) == "a b");
    CHECK(s.row_text(1) == "1 2");
}

TEST_CASE("edn text-input renders value and cursor")
{
    const Surface s = render(R"((text-input "ab" :cursor 0))", Size{ 3, 1 });
    CHECK(s.row_text(0) == "ab ");
    CHECK(has(s.at(Point{ 0, 0 }).style.attributes(), Attr::reverse));
}

TEST_CASE("edn viewport scrolls to the offset")
{
    const Surface s = render(R"((viewport ["a" "b" "c"] :offset 1))", Size{ 1, 2 });
    CHECK(s.row_text(0) == "b");
    CHECK(s.row_text(1) == "c");
}

TEST_CASE("edn drawing-area and tile-map build without errors")
{
    CHECK_NOTHROW(render(R"((drawing-area :content-size [4 4] :offset [1 1]))", Size{ 2, 2 }));
    CHECK_NOTHROW(render(R"((tile-map :map-size [2 2] :tile-size [3 3] :selected [1 1]))", Size{ 6, 6 }));
}

TEST_CASE("edn conversion helpers cover color, align, border and constraint")
{
    using edn_ui::to_align;
    using edn_ui::to_border;
    using edn_ui::to_color;
    using edn_ui::to_constraint;

    CHECK(to_color(edn::parse("[1 2 3]")) == Color::rgb(1, 2, 3));
    CHECK(to_color(edn::parse("42")) == Color::indexed(42));
    CHECK(to_color(edn::parse(":accent")) == palette::accent());
    CHECK(to_color(edn::parse("nil")) == Color::unset());

    CHECK(to_align(edn::parse(":center")) == Align::center);
    CHECK(to_align(edn::parse(":right")) == Align::end);

    CHECK(to_border(edn::parse(":thick")).top == Glyph{ "━" });
    CHECK(to_border(edn::parse(R"({:top "="})")).top == Glyph{ "=" });

    CHECK(to_constraint(edn::parse("3")).type() == Constraint::Kind::fixed);
    CHECK(to_constraint(edn::parse("[:percent 50]")).value() == 50);
}

TEST_CASE("edn invalid forms throw descriptive errors")
{
    CHECK_THROWS_AS(edn_ui::parse("(bogus)"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse("(text :align :diagonal)"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse("(text :wobble 1)"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse("(text :align)"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse(R"((text "x" :style {:fg "#zzz"}))"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse("(block (text \"a\") (text \"b\"))"), edn_ui::error);
    CHECK_THROWS_AS(edn_ui::parse("42"), edn_ui::error);
}

TEST_CASE("edn nested layout matches the counter example card")
{
    const Surface s = render(
        R"((column
             (block :size 3 :border :rounded :title " t " :title-align :center
               (text "7" :align :center))
             (progress 1.0 :size 1 :glyphs ["#" "-"])))",
        Size{ 7, 4 });
    CHECK(s.row_text(0) == "╭─ t ─╮");
    CHECK(s.row_text(1) == "│  7  │");
    CHECK(s.row_text(2) == "╰─────╯");
    CHECK(s.row_text(3) == "#######");
}
