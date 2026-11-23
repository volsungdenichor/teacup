#pragma once

/// Declarative widget trees described in EDN (https://github.com/edn-format/edn).
///
/// Requires the `edn` library on the include path; enabled with the
/// `TEACUP_WITH_EDN` CMake option, which points at a sibling checkout by default.
///
/// Example:
///
///     const auto ui = teacup::edn_ui::parse(R"(
///         (column
///           (block :size 9 :border :rounded :title " teacup " :title-align :center
///             (text "counter" :align :center :style {:fg :subtle}))
///           (progress 0.42 :size 1 :gradient ["#7d56f4" "#43bf6d"])
///           (text "q quit" :align :center :style {:bg :accent})))
///     )");
///     ui.render(canvas);

#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <edn/edn.hpp>

#include "teacup/border.hpp"
#include "teacup/canvas.hpp"
#include "teacup/color.hpp"
#include "teacup/geometry.hpp"
#include "teacup/layout.hpp"
#include "teacup/style.hpp"

namespace teacup
{
namespace edn_ui
{

/// Thrown when an EDN value does not describe a valid widget tree.
class error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/// A renderable widget built from an EDN description.
///
/// `size_hint` is the layout constraint a parent `row`/`column` uses for this
/// widget; it is set with the `:size` option and defaults to `flex`.
class Widget
{
public:
    using RenderFn = std::function<void(Canvas)>;

    Widget() = default;

    Widget(RenderFn render_fn, Constraint size) : render_{ std::move(render_fn) }, size_{ size } { }

    void render(Canvas target) const
    {
        if (render_ && target.valid())
        {
            render_(target);
        }
    }

    [[nodiscard]] Constraint size_hint() const { return size_; }

private:
    RenderFn render_;
    Constraint size_ = Constraint::flex();
};

/// Builds a widget tree from a parsed EDN form, e.g. `(text "hi" :align :center)`.
/// A bare string is shorthand for a `text` widget. Throws `error` on invalid input.
Widget build(const ::edn::value_t& value);

/// Parses EDN text containing a single widget form and builds it.
/// Throws `edn::parse_error` on malformed EDN and `error` on invalid widget forms.
Widget parse(std::string_view text);

/// Conversions from EDN values to teacup types. All throw `error` on mismatch.
///
/// Color:      "#7d56f4" | [r g b] | 42 (indexed) | :accent/:subtle/:text/:success/
///             :warning/:danger/:surface | :unset | nil
/// Style:      {:fg <color> :bg <color> :bold true :dim true :italic true
///              :underline true :blink true :reverse true :strikethrough true}
/// Align:      :start/:left/:top | :center | :end/:right/:bottom
/// Border:     :plain/:rounded/:thick/:double/:ascii/:blank or
///             {:top "─" :top-left "┌" ...} (unset glyphs fall back to plain)
/// Constraint: 3 (fixed) | :flex | [:fixed 3] | [:percent 50] | [:flex 2] | [:min 4]
Color to_color(const ::edn::value_t& value);
Style to_style(const ::edn::value_t& value);
Align to_align(const ::edn::value_t& value);
Border to_border(const ::edn::value_t& value);
Constraint to_constraint(const ::edn::value_t& value);

}  // namespace edn_ui
}  // namespace teacup
