#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace teacup
{

class Glyph
{
public:
    Glyph() = default;

    Glyph(const std::string& text) : Glyph{ from_utf8(text) } { }
    Glyph(std::string_view text) : Glyph{ from_utf8(text) } { }
    Glyph(const char* text) : Glyph{ std::string_view{ text } } { }
    static Glyph from_utf8(std::string_view text);

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

    GlyphString(const std::string& text) : GlyphString{ from_utf8(text) } { }
    GlyphString(const char* text) : GlyphString{ from_utf8(text) } { }
    GlyphString(std::string_view text) : GlyphString{ from_utf8(text) } { }

    static GlyphString from_utf8(std::string_view text);
    [[nodiscard]] std::string to_utf8() const;

    template <typename... Args>
    static GlyphString combine(Args&&... arg)
    {
        GlyphString result;
        ((result += GlyphString{ std::forward<Args>(arg) }), ...);
        return result;
    }

    [[nodiscard]] int width() const;

    [[nodiscard]] GlyphString truncate(int columns) const;

    [[nodiscard]] GlyphString substr(std::size_t pos, std::size_t count = std::string::npos) const;

    [[nodiscard]] std::size_t index_at_column(int column) const;

    [[nodiscard]] std::size_t size() const { return glyphs_.size(); }
    [[nodiscard]] bool empty() const { return glyphs_.empty(); }

    const Glyph& operator[](std::size_t index) const { return glyphs_[index]; }
    Glyph& operator[](std::size_t index) { return glyphs_[index]; }
    [[nodiscard]] const Glyph& back() const { return glyphs_.back(); }

    iterator begin() { return glyphs_.begin(); }
    iterator end() { return glyphs_.end(); }
    [[nodiscard]] const_iterator begin() const { return glyphs_.begin(); }
    [[nodiscard]] const_iterator end() const { return glyphs_.end(); }

    void push_back(const Glyph& glyph) { glyphs_.push_back(glyph); }
    void pop_back() { glyphs_.pop_back(); }
    void clear() { glyphs_.clear(); }
    iterator insert(const_iterator pos, const Glyph& glyph) { return glyphs_.insert(pos, glyph); }
    iterator erase(const_iterator pos) { return glyphs_.erase(pos); }

    GlyphString& operator+=(const GlyphString& other)
    {
        glyphs_.insert(glyphs_.end(), other.glyphs_.begin(), other.glyphs_.end());
        return *this;
    }

    friend GlyphString operator+(GlyphString lhs, const GlyphString& rhs) { return lhs += rhs; }

    friend bool operator==(const GlyphString& lhs, const GlyphString& rhs) { return lhs.glyphs_ == rhs.glyphs_; }
    friend bool operator!=(const GlyphString& lhs, const GlyphString& rhs) { return !(lhs == rhs); }

private:
    std::vector<Glyph> glyphs_;
};

inline Glyph operator""_g(const char* text, std::size_t)
{
    return Glyph::from_utf8(text);
}

inline GlyphString operator""_gs(const char* text, std::size_t)
{
    return GlyphString::from_utf8(text);
}

}  // namespace teacup
