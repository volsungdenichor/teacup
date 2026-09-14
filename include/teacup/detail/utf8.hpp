#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace teacup
{
namespace utf8
{

/// Decodes one code point starting at `pos`; advances `pos` past it.
/// Invalid bytes decode to U+FFFD and advance by one byte.
char32_t decode(const std::string& text, std::size_t& pos);

/// Appends the UTF-8 encoding of `cp` to `out`.
void encode(char32_t cp, std::string& out);

/// Number of terminal columns occupied by `cp` (0, 1 or 2).
int width(char32_t cp);

/// Number of terminal columns occupied by the whole string.
int width(const std::string& text);

/// Splits a string into code points.
std::vector<char32_t> to_code_points(const std::string& text);

}  // namespace utf8
}  // namespace teacup
