#include <catch2/catch_test_macros.hpp>

#include "teacup/detail/utf8.hpp"
#include "teacup/widgets/text.hpp"

using namespace teacup;

TEST_CASE("utf8 width accounts for wide and zero width code points")
{
    CHECK(utf8::width(std::string{ "abc" }) == 3);
    CHECK(utf8::width(std::string{ "\xe6\x97\xa5\xe6\x9c\xac" }) == 4);
    CHECK(utf8::width(std::string{ "e\xcc\x81" }) == 1);
}

TEST_CASE("wrap_text breaks on spaces")
{
    const auto lines = widgets::wrap_text("the quick brown fox", 10);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "the quick");
    CHECK(lines[1] == "brown fox");
}

TEST_CASE("wrap_text honours explicit newlines")
{
    const auto lines = widgets::wrap_text("a\nb", 10);
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "a");
    CHECK(lines[1] == "b");
}

TEST_CASE("text widget aligns content")
{
    surface s{ size{ 7, 1 } };
    s.clear();
    canvas c{ s };
    widgets::text{ "ab" }.align(align::center).render(c);

    CHECK(s.row_text(0) == "  ab   ");
}

TEST_CASE("text widget wraps and vertically aligns")
{
    surface s{ size{ 5, 3 } };
    s.clear();
    canvas c{ s };
    widgets::text{ "aa bb" }.wrap(true).vertical_align(align::end).render(c);

    CHECK(s.row_text(0) == "     ");
    CHECK(s.row_text(2) == "aa bb");
}
