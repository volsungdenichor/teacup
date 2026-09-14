#pragma once

#include <vector>

#include "teacup/geometry.hpp"

namespace teacup
{

/// A sizing rule for one slot of a split.
class constraint
{
public:
    enum class kind
    {
        fixed,
        percent,
        flex,
        min
    };

    static constraint fixed(int cells)
    {
        return { kind::fixed, cells };
    }
    static constraint percent(int value)
    {
        return { kind::percent, value };
    }
    /// Shares the remaining space proportionally to `weight`.
    static constraint flex(int weight = 1)
    {
        return { kind::flex, weight };
    }
    /// Like `flex`, but never smaller than `cells`.
    static constraint min(int cells)
    {
        return { kind::min, cells };
    }

    [[nodiscard]] kind type() const
    {
        return kind_;
    }
    [[nodiscard]] int value() const
    {
        return value_;
    }

private:
    constraint(kind k, int v)
        : kind_{ k }
        , value_{ v }
    {
    }

    kind kind_;
    int value_;
};

/// Splits `area` into vertical columns following `constraints`.
std::vector<rect> split_horizontal(const rect& area, const std::vector<constraint>& constraints, int gap = 0);

/// Splits `area` into horizontal rows following `constraints`.
std::vector<rect> split_vertical(const rect& area, const std::vector<constraint>& constraints, int gap = 0);

/// Centers a `width` x `height` box inside `area`.
rect centered(const rect& area, int width, int height);

}  // namespace teacup
