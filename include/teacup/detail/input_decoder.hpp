#pragma once

#include <string>
#include <vector>

#include "teacup/event.hpp"
#include "teacup/message.hpp"

namespace teacup
{
namespace detail
{

/// Incremental parser turning a terminal byte stream into event messages.
///
/// It keeps partial sequences in an internal buffer, so it can be fed with
/// arbitrary chunks. It is pure and therefore unit-testable without a tty.
class input_decoder
{
public:
    /// Appends `bytes` and returns every event that became complete.
    std::vector<message> feed(const std::string& bytes);

    /// Flushes a pending lone ESC as an escape key (call on input idle).
    std::vector<message> flush();

private:
    std::string buffer_{};
    bool in_paste_ = false;
    std::string paste_{};
};

}  // namespace detail
}  // namespace teacup
