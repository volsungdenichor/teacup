#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace teacup
{
namespace utf8
{

char32_t decode(const std::string& text, std::size_t& pos);

void encode(char32_t cp, std::string& out);

int width(char32_t cp);

int width(const std::string& text);

std::vector<char32_t> to_code_points(const std::string& text);

}  // namespace utf8
}  // namespace teacup
