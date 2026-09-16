#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "teacup/canvas.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace widgets
{

struct viewport_state
{
    std::vector<std::string> lines;
    int offset = 0;

    void clamp_offset()
    {
        offset = std::clamp(offset, 0, static_cast<int>(lines.size()));
    }
};

/// View and interaction logic for an application-owned viewport_state.
class viewport_view
{
public:
    using renderer_fn = std::function<void(const viewport_state&, canvas)>;

    viewport_view& style(const teacup::style& value)
    {
        style_ = value;
        return *this;
    }

    viewport_view& renderer(renderer_fn value)
    {
        renderer_ = std::move(value);
        return *this;
    }

    void scroll(viewport_state& state, int amount) const
    {
        state.offset += amount;
        state.clamp_offset();
    }
    void page_down(viewport_state& state, int height) const { scroll(state, (std::max)(1, height)); }
    void page_up(viewport_state& state, int height) const { scroll(state, -((std::max)(1, height))); }

    bool update(viewport_state& state, const mouse_event& event, const rect& area) const
    {
        if (!area.contains(event.position))
        {
            return false;
        }
        if (event.button == mouse_button::wheel_up)
        {
            scroll(state, -1);
            return true;
        }
        if (event.button == mouse_button::wheel_down)
        {
            scroll(state, 1);
            return true;
        }
        return false;
    }

    void render(const viewport_state& state, canvas target) const
    {
        if (renderer_)
        {
            renderer_(state, target);
            return;
        }
        if (!target.valid() || target.width() <= 0 || target.height() <= 0)
        {
            return;
        }
        const int first = std::min(state.offset, (std::max)(0, static_cast<int>(state.lines.size()) - target.height()));
        for (int row = 0; row < target.height(); ++row)
        {
            target.fill(rect{ 0, row, target.width(), 1 }, " ");
            const int index = first + row;
            if (index >= 0 && index < static_cast<int>(state.lines.size()))
            {
                target.text(0, row, state.lines[static_cast<std::size_t>(index)], style_);
            }
        }
    }

private:
    teacup::style style_;
    renderer_fn renderer_;
};

}  // namespace widgets
}  // namespace teacup