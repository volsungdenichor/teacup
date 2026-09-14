#pragma once

#include <any>
#include <type_traits>
#include <utility>

namespace teacup
{

/// A type-erased, copyable value passed to `update`.
class message
{
public:
    message() = default;

    template <
        typename T,
        typename = std::enable_if_t<!std::is_same<std::decay_t<T>, message>::value>>
    message(T&& value)  // NOLINT(google-explicit-constructor): implicit is the point
        : value_{ std::forward<T>(value) }
    {
    }

    [[nodiscard]] bool empty() const
    {
        return !value_.has_value();
    }

    template <typename T>
    [[nodiscard]] bool is() const
    {
        return value_.type() == typeid(T);
    }

    template <typename T>
    [[nodiscard]] const T* get() const
    {
        return std::any_cast<T>(&value_);
    }

private:
    std::any value_{};
};

/// Returns a pointer to the payload if `msg` holds a `T`, otherwise nullptr.
template <typename T>
const T* msg_cast(const message& msg)
{
    return msg.template get<T>();
}

}  // namespace teacup
