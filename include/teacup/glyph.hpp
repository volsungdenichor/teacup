#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace teacup
{

// One terminal character: a grapheme cluster with its cached display width (0, 1 or 2).
class Glyph
{
public:
    Glyph() = default;

    // Takes the first grapheme cluster of `text`; empty text yields a continuation.
    static Glyph from_utf8(std::string_view text);

    // Right half of a wide glyph.
    static Glyph continuation();

    [[nodiscard]] std::string_view utf8() const;
    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] bool is_continuation() const { return size_ == 0; }

    friend bool operator==(const Glyph& lhs, const Glyph& rhs)
    {
        return lhs.size_ == rhs.size_ && lhs.width_ == rhs.width_ && lhs.bytes_ == rhs.bytes_;
    }

    friend bool operator!=(const Glyph& lhs, const Glyph& rhs) { return !(lhs == rhs); }

private:
    // Clusters longer than the inline buffer are interned; bytes_ then holds the pool pointer.
    static constexpr std::uint8_t interned = 0xFF;

    std::array<char, 14> bytes_{ ' ' };
    std::uint8_t size_ = 1;
    std::uint8_t width_ = 1;
};

static_assert(sizeof(Glyph) == 16, "Glyph is meant to stay compact");

class GlyphString
{
public:
    using value_type = Glyph;
    using iterator = std::vector<Glyph>::iterator;
    using const_iterator = std::vector<Glyph>::const_iterator;

    GlyphString() = default;
    explicit GlyphString(std::vector<Glyph> glyphs) : glyphs_{ std::move(glyphs) } { }

    static GlyphString from_utf8(std::string_view text);
    [[nodiscard]] std::string to_utf8() const;

    [[nodiscard]] int width() const;

    // Longest prefix that fits in `columns`; never splits a wide glyph.
    [[nodiscard]] GlyphString truncate(int columns) const;

    [[nodiscard]] GlyphString substr(std::size_t pos, std::size_t count = std::string::npos) const;

    // Index of the glyph covering `column`, or size() past the end.
    [[nodiscard]] std::size_t index_at_column(int column) const;

    [[nodiscard]] std::size_t size() const { return glyphs_.size(); }
    [[nodiscard]] bool empty() const { return glyphs_.empty(); }

    const Glyph& operator[](std::size_t index) const { return glyphs_[index]; }
    Glyph& operator[](std::size_t index) { return glyphs_[index]; }

    iterator begin() { return glyphs_.begin(); }
    iterator end() { return glyphs_.end(); }
    [[nodiscard]] const_iterator begin() const { return glyphs_.begin(); }
    [[nodiscard]] const_iterator end() const { return glyphs_.end(); }

    void push_back(const Glyph& glyph) { glyphs_.push_back(glyph); }
    iterator insert(const_iterator pos, const Glyph& glyph) { return glyphs_.insert(pos, glyph); }
    iterator erase(const_iterator pos) { return glyphs_.erase(pos); }

    friend bool operator==(const GlyphString& lhs, const GlyphString& rhs) { return lhs.glyphs_ == rhs.glyphs_; }
    friend bool operator!=(const GlyphString& lhs, const GlyphString& rhs) { return !(lhs == rhs); }

private:
    std::vector<Glyph> glyphs_;
};

}  // namespace teacup
