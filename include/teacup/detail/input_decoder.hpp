#pragma once

#include <string>
#include <vector>

#include "teacup/message.hpp"

namespace teacup
{
namespace detail
{

class InputDecoder
{
public:
    std::vector<Message> feed(const std::string& bytes);

    std::vector<Message> flush();

private:
    std::string buffer_{};
    bool in_paste_ = false;
    std::string paste_{};
};

}  // namespace detail
}  // namespace teacup
