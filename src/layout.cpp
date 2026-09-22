#include "teacup/layout.hpp"

#include <numeric>

namespace teacup
{
namespace
{

std::vector<int> distribute(int total, const std::vector<Constraint>& constraints, int gap)
{
    const auto count = constraints.size();
    std::vector<int> sizes(count, 0);
    if (count == 0)
    {
        return sizes;
    }

    int available = total - gap * static_cast<int>(count - 1);
    if (available < 0)
    {
        available = 0;
    }

    int used = 0;
    int flex_weight = 0;
    for (std::size_t i = 0; i < count; ++i)
    {
        const auto& c = constraints[i];
        switch (c.type())
        {
            case Constraint::Kind::fixed: sizes[i] = (std::max)(0, c.value()); break;
            case Constraint::Kind::percent: sizes[i] = (std::max)(0, available * c.value() / 100); break;
            case Constraint::Kind::min:
                sizes[i] = (std::max)(0, c.value());
                flex_weight += 1;
                break;
            case Constraint::Kind::flex:
                sizes[i] = 0;
                flex_weight += (std::max)(1, c.value());
                break;
        }
        used += sizes[i];
    }

    int remaining = available - used;
    if (remaining > 0 && flex_weight > 0)
    {
        int handed_out = 0;
        std::size_t last_flex = count;
        for (std::size_t i = 0; i < count; ++i)
        {
            const auto kind = constraints[i].type();
            if (kind != Constraint::Kind::flex && kind != Constraint::Kind::min)
            {
                continue;
            }
            const int weight = kind == Constraint::Kind::min ? 1 : (std::max)(1, constraints[i].value());
            const int share = remaining * weight / flex_weight;
            sizes[i] += share;
            handed_out += share;
            last_flex = i;
        }
        if (last_flex < count)
        {
            sizes[last_flex] += remaining - handed_out;  // absorb rounding loss
        }
    }
    else if (remaining < 0)
    {
        for (std::size_t i = count; i-- > 0 && remaining < 0;)
        {
            const int take = (std::min)(sizes[i], -remaining);
            sizes[i] -= take;
            remaining += take;
        }
    }

    return sizes;
}

}  // namespace

std::vector<Rect> split_horizontal(const Rect& area, const std::vector<Constraint>& constraints, int gap)
{
    const std::vector<int> sizes = distribute(area.width(), constraints, gap);
    std::vector<Rect> result;
    result.reserve(sizes.size());
    int cursor = area.left();
    for (const int w : sizes)
    {
        result.push_back(Rect{ Point{ cursor, area.top() }, Size{ w, area.height() } });
        cursor += w + gap;
    }
    return result;
}

std::vector<Rect> split_vertical(const Rect& area, const std::vector<Constraint>& constraints, int gap)
{
    const std::vector<int> sizes = distribute(area.height(), constraints, gap);
    std::vector<Rect> result;
    result.reserve(sizes.size());
    int cursor = area.top();
    for (const int h : sizes)
    {
        result.push_back(Rect{ Point{ area.left(), cursor }, Size{ area.width(), h } });
        cursor += h + gap;
    }
    return result;
}

Rect centered(const Rect& area, int width, int height)
{
    const int w = (std::min)(width, area.width());
    const int h = (std::min)(height, area.height());
    return Rect{ area.left() + align_offset(Align::center, area.width(), w),
                 area.top() + align_offset(Align::center, area.height(), h),
                 w,
                 h };
}

}  // namespace teacup
