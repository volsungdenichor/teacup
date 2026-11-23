#include "teacup/edn_ui.hpp"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "teacup/glyph.hpp"
#include "teacup/widgets/block.hpp"
#include "teacup/widgets/checkbox.hpp"
#include "teacup/widgets/drawing_area.hpp"
#include "teacup/widgets/list.hpp"
#include "teacup/widgets/progress.hpp"
#include "teacup/widgets/spinner.hpp"
#include "teacup/widgets/table.hpp"
#include "teacup/widgets/tabs.hpp"
#include "teacup/widgets/text.hpp"
#include "teacup/widgets/text_input.hpp"
#include "teacup/widgets/tile_map.hpp"
#include "teacup/widgets/viewport.hpp"

namespace teacup
{
namespace edn_ui
{

namespace
{

using ::edn::value_t;

[[noreturn]] void fail(const std::string& message)
{
    throw error{ message };
}

std::string describe(const value_t& value)
{
    return ::edn::str(value.type(), " `", value, "`");
}

int to_int(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_integer())
    {
        return *v;
    }
    fail(::edn::str(what, ": expected an integer, got ", describe(value)));
}

double to_double(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_floating_point())
    {
        return *v;
    }
    if (const auto* v = value.if_integer())
    {
        return static_cast<double>(*v);
    }
    fail(::edn::str(what, ": expected a number, got ", describe(value)));
}

bool to_bool(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_boolean())
    {
        return *v;
    }
    fail(::edn::str(what, ": expected a boolean, got ", describe(value)));
}

const std::string& to_string(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_string())
    {
        return *v;
    }
    fail(::edn::str(what, ": expected a string, got ", describe(value)));
}

const ::edn::keyword_t& to_keyword(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_keyword())
    {
        return *v;
    }
    fail(::edn::str(what, ": expected a keyword, got ", describe(value)));
}

const ::edn::vector_t& to_vector(const value_t& value, std::string_view what)
{
    if (const auto* v = value.if_vector())
    {
        return *v;
    }
    fail(::edn::str(what, ": expected a vector, got ", describe(value)));
}

std::vector<std::string> to_strings(const value_t& value, std::string_view what)
{
    std::vector<std::string> result;
    for (const value_t& item : to_vector(value, what))
    {
        result.push_back(to_string(item, what));
    }
    return result;
}

std::vector<GlyphString> to_glyph_strings(const value_t& value, std::string_view what)
{
    std::vector<GlyphString> result;
    for (const value_t& item : to_vector(value, what))
    {
        result.emplace_back(to_string(item, what));
    }
    return result;
}

Size to_size(const value_t& value, std::string_view what)
{
    const auto& items = to_vector(value, what);
    if (items.size() != 2)
    {
        fail(::edn::str(what, ": expected [width height], got ", describe(value)));
    }
    return Size{ to_int(items[0], what), to_int(items[1], what) };
}

Point to_point(const value_t& value, std::string_view what)
{
    const auto& items = to_vector(value, what);
    if (items.size() != 2)
    {
        fail(::edn::str(what, ": expected [x y], got ", describe(value)));
    }
    return Point{ to_int(items[0], what), to_int(items[1], what) };
}

std::uint8_t to_component(const value_t& value, std::string_view what)
{
    const int v = to_int(value, what);
    if (v < 0 || v > 255)
    {
        fail(::edn::str(what, ": color component out of range [0, 255]: ", v));
    }
    return static_cast<std::uint8_t>(v);
}

/// Positional arguments and keyword options of a widget form.
struct form_args_t
{
    std::string name;
    std::vector<const value_t*> positional;
    std::vector<std::pair<std::string, const value_t*>> options;

    [[nodiscard]] const value_t* option(const std::string& key) const
    {
        for (const auto& [name_, value] : options)
        {
            if (name_ == key)
            {
                return value;
            }
        }
        return nullptr;
    }

    void check_options(std::initializer_list<const char*> allowed) const
    {
        for (const auto& option_ : options)
        {
            const std::string& key = option_.first;
            if (key == "size")
            {
                continue;
            }
            if (std::none_of(allowed.begin(), allowed.end(), [&key](const char* a) { return key == a; }))
            {
                fail(::edn::str("(", name, "): unknown option :", key));
            }
        }
    }

    [[nodiscard]] Constraint size_hint() const
    {
        if (const value_t* size = option("size"))
        {
            return to_constraint(*size);
        }
        return Constraint::flex();
    }
};

form_args_t split_form(const std::string& name, const ::edn::list_t& form)
{
    form_args_t result;
    result.name = name;
    for (std::size_t i = 1; i < form.size(); ++i)
    {
        if (const auto* key = form[i].if_keyword())
        {
            if (i + 1 >= form.size())
            {
                fail(::edn::str("(", name, "): missing value for option :", *key));
            }
            result.options.emplace_back(static_cast<const std::string&>(*key), &form[i + 1]);
            ++i;
        }
        else
        {
            result.positional.push_back(&form[i]);
        }
    }
    return result;
}

Widget make(std::function<void(Canvas)> render, const form_args_t& args)
{
    return Widget{ std::move(render), args.size_hint() };
}

Widget build_text(const form_args_t& args)
{
    args.check_options({ "style", "align", "vertical-align", "wrap" });
    widgets::Text widget;
    if (!args.positional.empty())
    {
        widget.content(to_string(*args.positional[0], "(text) content"));
    }
    if (const value_t* v = args.option("style"))
    {
        widget.style(to_style(*v));
    }
    if (const value_t* v = args.option("align"))
    {
        widget.align(to_align(*v));
    }
    if (const value_t* v = args.option("vertical-align"))
    {
        widget.vertical_align(to_align(*v));
    }
    if (const value_t* v = args.option("wrap"))
    {
        widget.wrap(to_bool(*v, "(text) :wrap"));
    }
    return make([widget](Canvas target) { widget.render(target); }, args);
}

Widget build_block(const form_args_t& args)
{
    args.check_options({ "border", "border-style", "title", "title-style", "title-align", "padding", "background" });
    widgets::Block widget;
    if (const value_t* v = args.option("border"))
    {
        widget.border(to_border(*v));
    }
    if (const value_t* v = args.option("border-style"))
    {
        widget.border_style(to_style(*v));
    }
    if (const value_t* v = args.option("title"))
    {
        widget.title(to_string(*v, "(block) :title"));
    }
    if (const value_t* v = args.option("title-style"))
    {
        widget.title_style(to_style(*v));
    }
    if (const value_t* v = args.option("title-align"))
    {
        widget.title_align(to_align(*v));
    }
    if (const value_t* v = args.option("padding"))
    {
        if (v->if_integer())
        {
            widget.padding(to_int(*v, "(block) :padding"));
        }
        else
        {
            const auto& items = to_vector(*v, "(block) :padding");
            if (items.size() != 4)
            {
                fail("(block): :padding expects an integer or [left top right bottom]");
            }
            const auto pad = [&](std::size_t i) { return to_int(items[i], "(block) :padding"); };
            widget.padding(pad(0), pad(1), pad(2), pad(3));
        }
    }
    if (const value_t* v = args.option("background"))
    {
        widget.background(to_color(*v));
    }
    if (args.positional.size() > 1)
    {
        fail("(block): expects at most one child; wrap several children in (column ...) or (row ...)");
    }
    if (args.positional.empty())
    {
        return make([widget](Canvas target) { widget.draw(target); }, args);
    }
    Widget child = build(*args.positional[0]);
    return make(
        [widget, child](Canvas target) { widget.render(target, [&](Canvas inner) { child.render(inner); }); }, args);
}

Widget build_split(const form_args_t& args, bool vertical)
{
    args.check_options({ "gap" });
    int gap = 0;
    if (const value_t* v = args.option("gap"))
    {
        gap = to_int(*v, ::edn::str("(", args.name, ") :gap"));
    }
    std::vector<Widget> children;
    std::vector<Constraint> constraints;
    for (const value_t* child : args.positional)
    {
        children.push_back(build(*child));
        constraints.push_back(children.back().size_hint());
    }
    return make(
        [children, constraints, gap, vertical](Canvas target)
        {
            const std::vector<Rect> rects = vertical ? split_vertical(target.bounds(), constraints, gap)
                                                     : split_horizontal(target.bounds(), constraints, gap);
            for (std::size_t i = 0; i < children.size() && i < rects.size(); ++i)
            {
                children[i].render(target.sub(rects[i]));
            }
        },
        args);
}

Widget build_centered(const form_args_t& args)
{
    args.check_options({});
    if (args.positional.size() != 3)
    {
        fail("(centered): expects width, height and a child, e.g. (centered 44 9 (block ...))");
    }
    const int width = to_int(*args.positional[0], "(centered) width");
    const int height = to_int(*args.positional[1], "(centered) height");
    Widget child = build(*args.positional[2]);
    return make(
        [width, height, child](Canvas target) { child.render(target.sub(centered(target.bounds(), width, height))); },
        args);
}

Widget build_spacer(const form_args_t& args)
{
    args.check_options({});
    return make([](Canvas) {}, args);
}

Widget build_progress(const form_args_t& args)
{
    args.check_options({ "gradient", "track-style", "glyphs" });
    widgets::Progress::State state;
    if (!args.positional.empty())
    {
        state.ratio = to_double(*args.positional[0], "(progress) ratio");
    }
    widgets::Progress::View view;
    if (const value_t* v = args.option("gradient"))
    {
        const auto& items = to_vector(*v, "(progress) :gradient");
        if (items.size() != 2)
        {
            fail("(progress): :gradient expects [from to]");
        }
        view.gradient(to_color(items[0]), to_color(items[1]));
    }
    if (const value_t* v = args.option("track-style"))
    {
        view.track_style(to_style(*v));
    }
    if (const value_t* v = args.option("glyphs"))
    {
        const auto& items = to_vector(*v, "(progress) :glyphs");
        if (items.size() != 2)
        {
            fail("(progress): :glyphs expects [filled empty]");
        }
        view.glyphs(to_string(items[0], "(progress) :glyphs"), to_string(items[1], "(progress) :glyphs"));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_checkbox(const form_args_t& args)
{
    args.check_options({ "checked-style", "unchecked-style", "glyphs" });
    widgets::Checkbox::State state;
    if (!args.positional.empty())
    {
        state.checked = to_bool(*args.positional[0], "(checkbox) checked");
    }
    widgets::Checkbox::View view;
    if (const value_t* v = args.option("checked-style"))
    {
        view.checked_style(to_style(*v));
    }
    if (const value_t* v = args.option("unchecked-style"))
    {
        view.unchecked_style(to_style(*v));
    }
    if (const value_t* v = args.option("glyphs"))
    {
        const auto& items = to_vector(*v, "(checkbox) :glyphs");
        if (items.size() != 2)
        {
            fail("(checkbox): :glyphs expects [checked unchecked]");
        }
        view.glyphs(to_string(items[0], "(checkbox) :glyphs"), to_string(items[1], "(checkbox) :glyphs"));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_spinner(const form_args_t& args)
{
    args.check_options({ "frames", "style" });
    widgets::Spinner::State state;
    if (!args.positional.empty())
    {
        state.frame = static_cast<std::size_t>(to_int(*args.positional[0], "(spinner) frame"));
    }
    widgets::Spinner::View view;
    if (const value_t* v = args.option("frames"))
    {
        view.frames(to_glyph_strings(*v, "(spinner) :frames"));
    }
    if (const value_t* v = args.option("style"))
    {
        view.style(to_style(*v));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_tabs(const form_args_t& args)
{
    args.check_options({ "selected", "tab-style", "selected-style" });
    widgets::Tabs::View view;
    if (!args.positional.empty())
    {
        view.labels(to_glyph_strings(*args.positional[0], "(tabs) labels"));
    }
    widgets::Tabs::State state;
    if (const value_t* v = args.option("selected"))
    {
        state.selected = to_int(*v, "(tabs) :selected");
    }
    if (const value_t* v = args.option("tab-style"))
    {
        view.tab_style(to_style(*v));
    }
    if (const value_t* v = args.option("selected-style"))
    {
        view.selected_style(to_style(*v));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_list(const form_args_t& args)
{
    args.check_options({ "selected", "offset", "selected-style", "item-style", "item-height" });
    widgets::List<std::string>::State state;
    if (!args.positional.empty())
    {
        state.items = to_strings(*args.positional[0], "(list) items");
    }
    if (const value_t* v = args.option("selected"))
    {
        state.selected = to_int(*v, "(list) :selected");
    }
    if (const value_t* v = args.option("offset"))
    {
        state.offset = to_int(*v, "(list) :offset");
    }
    widgets::List<std::string>::View view;
    if (const value_t* v = args.option("selected-style"))
    {
        view.selected_style(to_style(*v));
    }
    if (const value_t* v = args.option("item-style"))
    {
        view.item_style(to_style(*v));
    }
    if (const value_t* v = args.option("item-height"))
    {
        view.item_height(to_int(*v, "(list) :item-height"));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_table(const form_args_t& args)
{
    args.check_options({ "headers", "rows", "header-style", "row-style" });
    widgets::Table widget;
    const auto read_rows = [](const value_t& value)
    {
        std::vector<std::vector<GlyphString>> rows;
        for (const value_t& row : to_vector(value, "(table) rows"))
        {
            rows.push_back(to_glyph_strings(row, "(table) rows"));
        }
        return rows;
    };
    if (!args.positional.empty())
    {
        widget.rows(read_rows(*args.positional[0]));
    }
    if (const value_t* v = args.option("rows"))
    {
        widget.rows(read_rows(*v));
    }
    if (const value_t* v = args.option("headers"))
    {
        widget.headers(to_glyph_strings(*v, "(table) :headers"));
    }
    if (const value_t* v = args.option("header-style"))
    {
        widget.header_style(to_style(*v));
    }
    if (const value_t* v = args.option("row-style"))
    {
        widget.row_style(to_style(*v));
    }
    return make([widget](Canvas target) { widget.render(target); }, args);
}

Widget build_text_input(const form_args_t& args)
{
    args.check_options({ "cursor", "text-style", "cursor-style" });
    widgets::TextInput::State state;
    if (!args.positional.empty())
    {
        state.value = GlyphString{ to_string(*args.positional[0], "(text-input) value") };
    }
    state.cursor = state.value.size();
    if (const value_t* v = args.option("cursor"))
    {
        state.cursor = static_cast<std::size_t>(to_int(*v, "(text-input) :cursor"));
    }
    widgets::TextInput::View view;
    if (const value_t* v = args.option("text-style"))
    {
        view.text_style(to_style(*v));
    }
    if (const value_t* v = args.option("cursor-style"))
    {
        view.cursor_style(to_style(*v));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_viewport(const form_args_t& args)
{
    args.check_options({ "offset", "style" });
    widgets::Viewport::State state;
    if (!args.positional.empty())
    {
        state.lines = to_strings(*args.positional[0], "(viewport) lines");
    }
    if (const value_t* v = args.option("offset"))
    {
        state.offset = to_int(*v, "(viewport) :offset");
    }
    widgets::Viewport::View view;
    if (const value_t* v = args.option("style"))
    {
        view.style(to_style(*v));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_drawing_area(const form_args_t& args)
{
    args.check_options({ "content-size", "offset", "background", "scroll-step" });
    widgets::DrawingArea::State state;
    if (const value_t* v = args.option("offset"))
    {
        state.offset = to_point(*v, "(drawing-area) :offset");
    }
    widgets::DrawingArea::View view;
    if (const value_t* v = args.option("content-size"))
    {
        view.content_size(to_size(*v, "(drawing-area) :content-size"));
    }
    if (const value_t* v = args.option("background"))
    {
        view.background(to_style(*v));
    }
    if (const value_t* v = args.option("scroll-step"))
    {
        view.scroll_step(to_int(*v, "(drawing-area) :scroll-step"));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_tile_map(const form_args_t& args)
{
    args.check_options({ "map-size", "tile-size", "selected", "offset", "background", "selected-style" });
    widgets::TileMap::State state;
    if (const value_t* v = args.option("selected"))
    {
        state.selected = to_point(*v, "(tile-map) :selected");
    }
    if (const value_t* v = args.option("offset"))
    {
        state.drawing.offset = to_point(*v, "(tile-map) :offset");
    }
    widgets::TileMap::View view;
    if (const value_t* v = args.option("tile-size"))
    {
        view.tile_size(to_size(*v, "(tile-map) :tile-size"));
    }
    if (const value_t* v = args.option("map-size"))
    {
        view.map_size(to_size(*v, "(tile-map) :map-size"));
    }
    if (const value_t* v = args.option("background"))
    {
        view.background(to_style(*v));
    }
    if (const value_t* v = args.option("selected-style"))
    {
        view.selected_style(to_style(*v));
    }
    return make([view, state](Canvas target) { view.render(state, target); }, args);
}

Widget build_form(const ::edn::list_t& form)
{
    if (form.empty())
    {
        fail("widget form: expected (name args...), got an empty list");
    }
    const auto* name = form[0].if_symbol();
    if (!name)
    {
        fail(::edn::str("widget form: expected a symbol as the form head, got ", describe(form[0])));
    }

    using builder_fn = Widget (*)(const form_args_t&);
    static const std::vector<std::pair<std::string_view, builder_fn>> builders = {
        { "text", build_text },
        { "block", build_block },
        { "centered", build_centered },
        { "spacer", build_spacer },
        { "progress", build_progress },
        { "checkbox", build_checkbox },
        { "spinner", build_spinner },
        { "tabs", build_tabs },
        { "list", build_list },
        { "table", build_table },
        { "text-input", build_text_input },
        { "viewport", build_viewport },
        { "drawing-area", build_drawing_area },
        { "tile-map", build_tile_map },
    };

    if (*name == "row" || *name == "column")
    {
        return build_split(split_form(*name, form), *name == "column");
    }
    for (const auto& [key, builder] : builders)
    {
        if (*name == key)
        {
            return builder(split_form(*name, form));
        }
    }
    fail(::edn::str(
        "unknown widget form (", *name,
        "); expected one of: text, block, row, column, centered, spacer, progress, checkbox, spinner, tabs, list, "
        "table, text-input, viewport, drawing-area, tile-map"));
}

}  // namespace

Color to_color(const value_t& value)
{
    if (value.is_nil())
    {
        return Color::unset();
    }
    if (const auto* text = value.if_string())
    {
        const Color color = Color::hex(*text);
        if (!color.is_set())
        {
            fail(::edn::str("color: invalid hex value \"", *text, "\""));
        }
        return color;
    }
    if (value.if_integer())
    {
        return Color::indexed(to_component(value, "color"));
    }
    if (const auto* items = value.if_vector())
    {
        if (items->size() != 3)
        {
            fail(::edn::str("color: expected [r g b], got ", describe(value)));
        }
        return Color::rgb(
            to_component((*items)[0], "color"), to_component((*items)[1], "color"), to_component((*items)[2], "color"));
    }
    if (const auto* keyword = value.if_keyword())
    {
        static const std::vector<std::pair<std::string_view, Color (*)()>> named = {
            { "subtle", palette::subtle },   { "text", palette::text },       { "accent", palette::accent },
            { "success", palette::success }, { "warning", palette::warning }, { "danger", palette::danger },
            { "surface", palette::surface },
        };
        if (*keyword == "unset")
        {
            return Color::unset();
        }
        for (const auto& [name, color] : named)
        {
            if (*keyword == name)
            {
                return color();
            }
        }
        fail(::edn::str("color: unknown palette name :", *keyword));
    }
    fail(::edn::str("color: expected a hex string, [r g b], indexed integer or palette keyword, got ", describe(value)));
}

Style to_style(const value_t& value)
{
    const auto* map = value.if_map();
    if (!map)
    {
        fail(::edn::str("style: expected a map like {:fg :accent :bold true}, got ", describe(value)));
    }
    Style style;
    static const std::vector<std::pair<std::string_view, Attr>> attrs = {
        { "bold", Attr::bold },       { "dim", Attr::dim },
        { "italic", Attr::italic },   { "underline", Attr::underline },
        { "blink", Attr::blink },     { "reverse", Attr::reverse },
        { "strikethrough", Attr::strikethrough },
    };
    for (const auto& [key, item] : *map)
    {
        const auto& keyword = to_keyword(key, "style key");
        if (keyword == "fg")
        {
            style.fg(to_color(item));
        }
        else if (keyword == "bg")
        {
            style.bg(to_color(item));
        }
        else
        {
            const auto it = std::find_if(
                attrs.begin(), attrs.end(), [&keyword](const auto& attr) { return keyword == attr.first; });
            if (it == attrs.end())
            {
                fail(::edn::str("style: unknown key :", keyword));
            }
            if (to_bool(item, ::edn::str("style :", keyword)))
            {
                style.add(it->second);
            }
        }
    }
    return style;
}

Align to_align(const value_t& value)
{
    const auto& keyword = to_keyword(value, "align");
    if (keyword == "start" || keyword == "left" || keyword == "top")
    {
        return Align::start;
    }
    if (keyword == "center")
    {
        return Align::center;
    }
    if (keyword == "end" || keyword == "right" || keyword == "bottom")
    {
        return Align::end;
    }
    fail(::edn::str("align: expected :start, :center or :end, got :", keyword));
}

Border to_border(const value_t& value)
{
    if (const auto* keyword = value.if_keyword())
    {
        static const std::vector<std::pair<std::string_view, Border (*)()>> named = {
            { "plain", Border::plain },   { "rounded", Border::rounded },
            { "thick", Border::thick },   { "double", Border::double_line },
            { "ascii", Border::ascii },   { "blank", Border::blank },
        };
        for (const auto& [name, border] : named)
        {
            if (*keyword == name)
            {
                return border();
            }
        }
        fail(::edn::str("border: expected :plain, :rounded, :thick, :double, :ascii or :blank, got :", *keyword));
    }
    if (const auto* map = value.if_map())
    {
        Border border = Border::plain();
        static const std::vector<std::pair<std::string_view, Glyph Border::*>> glyphs = {
            { "top", &Border::top },
            { "bottom", &Border::bottom },
            { "left", &Border::left },
            { "right", &Border::right },
            { "top-left", &Border::top_left },
            { "top-right", &Border::top_right },
            { "bottom-left", &Border::bottom_left },
            { "bottom-right", &Border::bottom_right },
        };
        for (const auto& [key, item] : *map)
        {
            const auto& keyword = to_keyword(key, "border key");
            const auto it = std::find_if(
                glyphs.begin(), glyphs.end(), [&keyword](const auto& glyph) { return keyword == glyph.first; });
            if (it == glyphs.end())
            {
                fail(::edn::str("border: unknown key :", keyword));
            }
            border.*(it->second) = Glyph{ to_string(item, ::edn::str("border :", keyword)) };
        }
        return border;
    }
    fail(::edn::str("border: expected a keyword or a glyph map, got ", describe(value)));
}

Constraint to_constraint(const value_t& value)
{
    if (value.if_integer())
    {
        return Constraint::fixed(to_int(value, "size"));
    }
    if (const auto* keyword = value.if_keyword())
    {
        if (*keyword == "flex")
        {
            return Constraint::flex();
        }
        fail(::edn::str("size: expected :flex, got :", *keyword));
    }
    if (const auto* items = value.if_vector())
    {
        if (items->size() != 2)
        {
            fail(::edn::str("size: expected [:fixed|:percent|:flex|:min n], got ", describe(value)));
        }
        const auto& kind = to_keyword((*items)[0], "size kind");
        const int amount = to_int((*items)[1], "size amount");
        if (kind == "fixed")
        {
            return Constraint::fixed(amount);
        }
        if (kind == "percent")
        {
            return Constraint::percent(amount);
        }
        if (kind == "flex")
        {
            return Constraint::flex(amount);
        }
        if (kind == "min")
        {
            return Constraint::min(amount);
        }
        fail(::edn::str("size: unknown kind :", kind));
    }
    fail(::edn::str("size: expected an integer, :flex or [kind n], got ", describe(value)));
}

Widget build(const value_t& value)
{
    if (const auto* text = value.if_string())
    {
        const widgets::Text widget{ GlyphString{ *text } };
        return Widget{ [widget](Canvas target) { widget.render(target); }, Constraint::flex() };
    }
    if (const auto* form = value.if_list())
    {
        return build_form(*form);
    }
    fail(::edn::str("widget: expected a form like (text ...) or a string, got ", describe(value)));
}

Widget parse(std::string_view text)
{
    return build(::edn::parse(text));
}

}  // namespace edn_ui
}  // namespace teacup
