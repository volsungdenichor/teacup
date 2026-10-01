#include "teacup/glyph.hpp"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_set>

#include "teacup/detail/utf8.hpp"

namespace teacup
{
namespace
{

// Never shrinks: distinct long clusters (ZWJ emoji sequences) are few in practice.
const std::string* intern(std::string_view text)
{
    static std::mutex mutex;
    static std::unordered_set<std::string> pool;
    const std::lock_guard<std::mutex> lock{ mutex };
    return &*pool.emplace(text).first;
}

}  // namespace

Glyph Glyph::from_utf8(std::string_view text)
{
    if (text.empty())
    {
        return continuation();
    }
    const std::string_view cluster = text.substr(0, utf8::next_grapheme(text, 0));
    Glyph result;
    result.bytes_.fill('\0');
    result.width_ = static_cast<std::uint8_t>(utf8::grapheme_width(cluster));
    if (cluster.size() <= result.bytes_.size())
    {
        std::memcpy(result.bytes_.data(), cluster.data(), cluster.size());
        result.size_ = static_cast<std::uint8_t>(cluster.size());
    }
    else
    {
        const std::string* pooled = intern(cluster);
        std::memcpy(result.bytes_.data(), &pooled, sizeof pooled);
        result.size_ = interned;
    }
    return result;
}

Glyph Glyph::continuation()
{
    Glyph result;
    result.bytes_.fill('\0');
    result.size_ = 0;
    result.width_ = 0;
    return result;
}

std::string_view Glyph::utf8() const
{
    if (size_ == interned)
    {
        const std::string* pooled = nullptr;
        std::memcpy(&pooled, bytes_.data(), sizeof pooled);
        return *pooled;
    }
    return std::string_view{ bytes_.data(), size_ };
}

GlyphString GlyphString::from_utf8(std::string_view text)
{
    GlyphString result;
    std::size_t pos = 0;
    while (pos < text.size())
    {
        const std::size_t end = utf8::next_grapheme(text, pos);
        result.glyphs_.push_back(Glyph::from_utf8(text.substr(pos, end - pos)));
        pos = end;
    }
    return result;
}

std::string GlyphString::to_utf8() const
{
    std::string result;
    for (const Glyph& glyph : glyphs_)
    {
        result += glyph.utf8();
    }
    return result;
}

int GlyphString::width() const
{
    int total = 0;
    for (const Glyph& glyph : glyphs_)
    {
        total += glyph.width();
    }
    return total;
}

GlyphString GlyphString::truncate(int columns) const
{
    int used = 0;
    auto it = glyphs_.begin();
    while (it != glyphs_.end() && used + it->width() <= columns)
    {
        used += it->width();
        ++it;
    }
    return GlyphString{ std::vector<Glyph>(glyphs_.begin(), it) };
}

GlyphString GlyphString::substr(std::size_t pos, std::size_t count) const
{
    pos = (std::min)(pos, glyphs_.size());
    count = (std::min)(count, glyphs_.size() - pos);
    const auto first = glyphs_.begin() + static_cast<std::ptrdiff_t>(pos);
    return GlyphString{ std::vector<Glyph>(first, first + static_cast<std::ptrdiff_t>(count)) };
}

std::size_t GlyphString::index_at_column(int column) const
{
    int x = 0;
    for (std::size_t i = 0; i < glyphs_.size(); ++i)
    {
        x += glyphs_[i].width();
        if (column < x)
        {
            return i;
        }
    }
    return glyphs_.size();
}

}  // namespace teacup
