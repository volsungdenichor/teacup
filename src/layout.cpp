#include "teacup/layout.hpp"

#include <numeric>

namespace teacup
{
namespace
{

std::vector<int> distribute(int total, const std::vector<constraint>& constraints, int gap)
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
            case constraint::kind::fixed: sizes[i] = (std::max)(0, c.value()); break;
            case constraint::kind::percent: sizes[i] = (std::max)(0, available * c.value() / 100); break;
            case constraint::kind::min:
                sizes[i] = (std::max)(0, c.value());
                flex_weight += 1;
                break;
            case constraint::kind::flex:
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
            if (kind != constraint::kind::flex && kind != constraint::kind::min)
            {
                continue;
            }
            const int weight = kind == constraint::kind::min ? 1 : (std::max)(1, constraints[i].value());
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

std::vector<rect> split_horizontal(const rect& area, const std::vector<constraint>& constraints, int gap)
{
    const std::vector<int> sizes = distribute(area.width(), constraints, gap);
    std::vector<rect> result;
    result.reserve(sizes.size());
    int cursor = area.left();
    for (const int w : sizes)
    {
        result.push_back(rect{ point{ cursor, area.top() }, size{ w, area.height() } });
        cursor += w + gap;
    }
    return result;
}

std::vector<rect> split_vertical(const rect& area, const std::vector<constraint>& constraints, int gap)
{
    const std::vector<int> sizes = distribute(area.height(), constraints, gap);
    std::vector<rect> result;
    result.reserve(sizes.size());
    int cursor = area.top();
    for (const int h : sizes)
    {
        result.push_back(rect{ point{ area.left(), cursor }, size{ area.width(), h } });
        cursor += h + gap;
    }
    return result;
}

rect centered(const rect& area, int width, int height)
{
    const int w = (std::min)(width, area.width());
    const int h = (std::min)(height, area.height());
    return rect{ area.left() + align_offset(align::center, area.width(), w),
                 area.top() + align_offset(align::center, area.height(), h),
                 w,
                 h };
}

}  // namespace teacup
