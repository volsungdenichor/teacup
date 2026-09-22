#pragma once

#include <vector>

#include "teacup/geometry.hpp"

namespace teacup
{

class Constraint
{
public:
    enum class Kind
    {
        fixed,
        percent,
        flex,
        min
    };

    static Constraint fixed(int cells) { return { Kind::fixed, cells }; }
    static Constraint percent(int value) { return { Kind::percent, value }; }
    static Constraint flex(int weight = 1) { return { Kind::flex, weight }; }
    static Constraint min(int cells) { return { Kind::min, cells }; }

    [[nodiscard]] Kind type() const { return kind_; }
    [[nodiscard]] int value() const { return value_; }

private:
    Constraint(Kind k, int v) : kind_{ k }, value_{ v } { }

    Kind kind_;
    int value_;
};

std::vector<Rect> split_horizontal(const Rect& area, const std::vector<Constraint>& constraints, int gap = 0);

std::vector<Rect> split_vertical(const Rect& area, const std::vector<Constraint>& constraints, int gap = 0);

Rect centered(const Rect& area, int width, int height);

}  // namespace teacup
