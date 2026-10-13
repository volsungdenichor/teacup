#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <utility>

#include "teacup/canvas.hpp"
#include "teacup/event.hpp"
#include "teacup/style.hpp"
#include "teacup/surface.hpp"

namespace teacup
{
namespace widgets
{

/// A free-form drawing surface that may be larger than the area it is shown in.
///
/// The draw callback receives a `Canvas` spanning the whole content (`content_size`),
/// so all drawing happens in content coordinates; the view maps the scrolled window
/// of that content onto the target canvas.
struct DrawingArea
{
    struct State
    {
        /// Content coordinate shown at the top-left corner of the widget.
        Point offset{};

        void clamp_offset(const Size& content, const Size& viewport)
        {
            offset.x = std::clamp(offset.x, 0, (std::max)(0, content.width - viewport.width));
            offset.y = std::clamp(offset.y, 0, (std::max)(0, content.height - viewport.height));
        }
    };

    class View
    {
    public:
        using DrawFn = std::function<void(Canvas)>;
        using RendererFn = std::function<void(const State&, Canvas)>;

        View& content_size(const Size& value)
        {
            content_size_ = Size{ (std::max)(0, value.width), (std::max)(0, value.height) };
            return *this;
        }

        /// Size of the on-screen area; used to clamp scrolling and size pages in `update`.
        View& viewport_size(const Size& value)
        {
            viewport_size_ = Size{ (std::max)(0, value.width), (std::max)(0, value.height) };
            return *this;
        }

        View& draw(DrawFn value)
        {
            draw_ = std::move(value);
            return *this;
        }

        /// Style used to clear the content before drawing and to paint the area beyond it.
        View& background(const Style& value)
        {
            background_ = value;
            return *this;
        }

        View& scroll_step(int value)
        {
            scroll_step_ = (std::max)(1, value);
            return *this;
        }

        View& renderer(RendererFn value)
        {
            renderer_ = std::move(value);
            return *this;
        }

        [[nodiscard]] Size content_size() const { return content_size_; }
        [[nodiscard]] Size viewport_size() const { return viewport_size_; }

        /// Offset actually used when showing `state` in a viewport of the given size.
        [[nodiscard]] Point visible_offset(const State& state, const Size& viewport) const
        {
            State copy = state;
            copy.clamp_offset(content_size_, viewport);
            return copy.offset;
        }

        /// Part of the content (in content coordinates) visible in a viewport of the given size.
        [[nodiscard]] Rect visible_rect(const State& state, const Size& viewport) const
        {
            return Rect{ visible_offset(state, viewport), viewport }.intersect(Rect::from_size(content_size_));
        }

        void scroll(State& state, int dx, int dy) const
        {
            state.offset = state.offset + Point{ dx, dy };
            state.clamp_offset(content_size_, viewport_size_);
        }

        void scroll_to(State& state, const Point& offset) const
        {
            state.offset = offset;
            state.clamp_offset(content_size_, viewport_size_);
        }

        /// Scrolls the minimal amount needed to bring `target` (content coordinates) into view.
        void ensure_visible(State& state, const Rect& target) const
        {
            const auto adjust = [](int offset, int view, int lo, int hi)
            {
                if (hi - lo > view || lo < offset)
                {
                    return lo;
                }
                return hi > offset + view ? hi - view : offset;
            };
            state.clamp_offset(content_size_, viewport_size_);
            state.offset.x = adjust(state.offset.x, viewport_size_.width, target.left(), target.right());
            state.offset.y = adjust(state.offset.y, viewport_size_.height, target.top(), target.bottom());
            state.clamp_offset(content_size_, viewport_size_);
        }

        void page_up(State& state) const { scroll(state, 0, -page_height()); }
        void page_down(State& state) const { scroll(state, 0, page_height()); }

        bool update(State& state, const KeyEvent& event) const
        {
            const Point before = state.offset;
            if (event.matches_any({ "up", "k" }))
            {
                scroll(state, 0, -scroll_step_);
            }
            else if (event.matches_any({ "down", "j" }))
            {
                scroll(state, 0, scroll_step_);
            }
            else if (event.matches_any({ "left", "h" }))
            {
                scroll(state, -scroll_step_, 0);
            }
            else if (event.matches_any({ "right", "l" }))
            {
                scroll(state, scroll_step_, 0);
            }
            else if (event.matches("pgup"))
            {
                page_up(state);
            }
            else if (event.matches("pgdown"))
            {
                page_down(state);
            }
            else if (event.matches("home"))
            {
                scroll_to(state, Point{ 0, 0 });
            }
            else if (event.matches("end"))
            {
                scroll_to(state, Point{ state.offset.x, content_size_.height });
            }
            else
            {
                return false;
            }
            return state.offset != before;
        }

        /// Wheel scrolls vertically; shift+wheel scrolls horizontally.
        bool update(State& state, const MouseEvent& event, const Rect& area) const
        {
            if (!area.contains(event.position))
            {
                return false;
            }
            const int direction = event.button == MouseButton::wheel_up     ? -1
                                  : event.button == MouseButton::wheel_down ? 1
                                                                            : 0;
            if (direction == 0)
            {
                return false;
            }
            const Point before = state.offset;
            if (has(event.modifiers, Modifier::shift))
            {
                scroll(state, direction * scroll_step_, 0);
            }
            else
            {
                scroll(state, 0, direction * scroll_step_);
            }
            return state.offset != before;
        }

        /// Maps a screen position inside `area` to content coordinates, if it lands on content.
        [[nodiscard]] std::optional<Point> to_content(const State& state, const Point& screen, const Rect& area) const
        {
            if (!area.contains(screen))
            {
                return std::nullopt;
            }
            const Point p = screen - area.origin() + visible_offset(state, area.size());
            if (!Rect::from_size(content_size_).contains(p))
            {
                return std::nullopt;
            }
            return p;
        }

        /// Maps a content position to the screen, if it is currently visible inside `area`.
        [[nodiscard]] std::optional<Point> to_screen(const State& state, const Point& content, const Rect& area) const
        {
            if (!visible_rect(state, area.size()).contains(content))
            {
                return std::nullopt;
            }
            return content - visible_offset(state, area.size()) + area.origin();
        }

        void render(const State& state, Canvas target) const
        {
            if (renderer_)
            {
                renderer_(state, target);
                return;
            }
            if (!target.valid())
            {
                return;
            }

            Surface content{ content_size_ };
            content.clear(background_);
            if (draw_ && !content_size_.empty())
            {
                draw_(Canvas{ content });
            }

            const Size view = target.size();
            const Point offset = visible_offset(state, view);
            target.fill(Glyph{}, background_);
            for (int y = 0; y < view.height; ++y)
            {
                for (int x = 0; x < view.width; ++x)
                {
                    const Point source = offset + Point{ x, y };
                    if (!content.contains(source))
                    {
                        continue;
                    }
                    Cell cell = content.at(source);
                    // Wide glyphs cut by the left/right edge cannot be shown; replace their halves with blanks.
                    const bool cut_left = x == 0 && cell.glyph.is_continuation();
                    const bool cut_right = x == view.width - 1 && cell.glyph.width() > 1;
                    if (cut_left || cut_right)
                    {
                        cell.glyph = Glyph{};
                    }
                    target.put(Point{ x, y }, cell);
                }
            }
        }

    private:
        [[nodiscard]] int page_height() const { return (std::max)(1, viewport_size_.height); }

        Size content_size_{};
        Size viewport_size_{};
        Style background_{};
        int scroll_step_ = 1;
        DrawFn draw_;
        RendererFn renderer_;
    };
};

}  // namespace widgets
}  // namespace teacup
