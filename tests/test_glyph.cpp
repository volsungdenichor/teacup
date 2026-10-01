#include <catch2/catch_test_macros.hpp>

#include "teacup/glyph.hpp"

using namespace teacup;

TEST_CASE("default glyph is a single-width space")
{
    const Glyph g;
    CHECK(g.utf8() == " ");
    CHECK(g.width() == 1);
    CHECK_FALSE(g.is_continuation());
}

TEST_CASE("continuation glyph has no text and no width")
{
    const Glyph g = Glyph::continuation();
    CHECK(g.is_continuation());
    CHECK(g.width() == 0);
    CHECK(g.utf8().empty());
    CHECK(Glyph::from_utf8("") == g);
}

TEST_CASE("glyph takes only the first grapheme cluster")
{
    CHECK(Glyph::from_utf8("ab").utf8() == "a");
    CHECK(Glyph::from_utf8("e\xcc\x81x").utf8() == "e\xcc\x81");  // e + COMBINING ACUTE
    CHECK(Glyph::from_utf8("e\xcc\x81x").width() == 1);
    CHECK(Glyph::from_utf8("\r\nx").utf8() == "\r\n");
}

TEST_CASE("glyph widths follow the cluster, not the code points")
{
    CHECK(Glyph::from_utf8("\xe6\x97\xa5").width() == 2);                      // CJK U+65E5
    CHECK(Glyph::from_utf8("\xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd").width() == 2);  // thumbs up + skin tone
    CHECK(Glyph::from_utf8("\xe2\x9d\xa4\xef\xb8\x8f").width() == 2);          // heart + VS16
    CHECK(Glyph::from_utf8("\xe2\x9d\xa4").width() == 1);                      // heart, text presentation
    CHECK(Glyph::from_utf8("\t").width() == 0);
}

TEST_CASE("regional indicators pair into flags")
{
    const std::string pl = "\xf0\x9f\x87\xb5\xf0\x9f\x87\xb1";
    const std::string de = "\xf0\x9f\x87\xa9\xf0\x9f\x87\xaa";
    const GlyphString flags = GlyphString::from_utf8(pl + de);
    REQUIRE(flags.size() == 2);
    CHECK(flags[0].utf8() == pl);
    CHECK(flags[1].utf8() == de);
    CHECK(flags.width() == 4);
}

TEST_CASE("long ZWJ sequences are interned and compare by value")
{
    // man, ZWJ, woman, ZWJ, girl, ZWJ, boy: 25 bytes, longer than the inline buffer
    const std::string family = "\xf0\x9f\x91\xa8\xe2\x80\x8d\xf0\x9f\x91\xa9\xe2\x80\x8d"
                               "\xf0\x9f\x91\xa7\xe2\x80\x8d\xf0\x9f\x91\xa6";
    const Glyph a = Glyph::from_utf8(family);
    const Glyph b = Glyph::from_utf8(family + "x");
    CHECK(a.utf8() == family);
    CHECK(a.width() == 2);
    CHECK(a == b);
    CHECK(GlyphString::from_utf8(family).size() == 1);
}

TEST_CASE("glyph string round-trips and measures UTF-8 text")
{
    const std::string text = "ae\xcc\x81\xe6\x97\xa5";  // a, e + acute, CJK
    const GlyphString s = GlyphString::from_utf8(text);
    CHECK(s.size() == 3);
    CHECK(s.width() == 4);
    CHECK(s.to_utf8() == text);
}

TEST_CASE("glyph string truncation never splits a wide glyph")
{
    const GlyphString s = GlyphString::from_utf8("a\xe6\x97\xa5" "b");
    CHECK(s.truncate(3).to_utf8() == "a\xe6\x97\xa5");
    CHECK(s.truncate(2).to_utf8() == "a");
    CHECK(s.truncate(0).empty());
    CHECK(s.truncate(10) == s);
}

TEST_CASE("glyph string maps columns to glyph indices")
{
    const GlyphString s = GlyphString::from_utf8("a\xe6\x97\xa5" "b");
    CHECK(s.index_at_column(0) == 0);
    CHECK(s.index_at_column(1) == 1);
    CHECK(s.index_at_column(2) == 1);
    CHECK(s.index_at_column(3) == 2);
    CHECK(s.index_at_column(4) == 3);
}

TEST_CASE("glyph string supports editing")
{
    GlyphString s = GlyphString::from_utf8("ac");
    s.insert(s.begin() + 1, Glyph::from_utf8("b"));
    CHECK(s.to_utf8() == "abc");
    s.erase(s.begin());
    CHECK(s.to_utf8() == "bc");
}
