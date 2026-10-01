#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace teacup
{
namespace utf8
{

char32_t decode(std::string_view text, std::size_t& pos);

void encode(char32_t cp, std::string& out);

int width(char32_t cp);

// Display width of a whole string, summed per grapheme cluster.
int width(std::string_view text);

// End offset of the grapheme cluster starting at `pos` (pragmatic subset of UAX #29).
std::size_t next_grapheme(std::string_view text, std::size_t pos);

// Display width (0, 1 or 2) of a single grapheme cluster.
int grapheme_width(std::string_view cluster);

std::vector<char32_t> to_code_points(std::string_view text);

}  // namespace utf8
}  // namespace teacup
