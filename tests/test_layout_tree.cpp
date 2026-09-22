#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

#include "teacup/layout_tree.hpp"

using namespace teacup;

namespace
{

layout::Node sample_tree()
{
    using namespace layout;
    return column({
        fixed(7, named("header", inset(2, column({
            fixed(1, row({
                flex(leaf("title")),
                fixed(14, leaf("spinner")),
            })),
            flex(space()),
            fixed(1, leaf("tabs")),
        })))),
        flex(row({
            percent(42, named("navigation", inset(2, column({
                fixed(6, leaf("menu")),
                flex(leaf("players")),
            }, 1)))),
            flex(named("components", inset(2, column({
                fixed(1, leaf("progress")),
                fixed(4, leaf("table")),
                fixed(5, named("input-box", inset(1, leaf("input")))),
            }, 1)))),
        }, 1)),
        fixed(1, leaf("footer")),
    });
}

}  // namespace

TEST_CASE("a resolved tree matches hand-computed splits")
{
    const layout::Areas areas = layout::resolve(sample_tree(), Rect{ 0, 0, 80, 24 });

    CHECK(areas.get("header") == Rect{ 0, 0, 80, 7 });
    CHECK(areas.get("title") == Rect{ 2, 2, 62, 1 });
    CHECK(areas.get("spinner") == Rect{ 64, 2, 14, 1 });
    CHECK(areas.get("tabs") == Rect{ 2, 4, 76, 1 });
    CHECK(areas.get("navigation") == Rect{ 0, 7, 33, 16 });
    CHECK(areas.get("menu") == Rect{ 2, 9, 29, 6 });
    CHECK(areas.get("players") == Rect{ 2, 16, 29, 5 });
    CHECK(areas.get("components") == Rect{ 34, 7, 46, 16 });
    CHECK(areas.get("progress") == Rect{ 36, 9, 42, 1 });
    CHECK(areas.get("table") == Rect{ 36, 11, 42, 4 });
    CHECK(areas.get("input-box") == Rect{ 36, 16, 42, 5 });
    CHECK(areas.get("input") == Rect{ 37, 17, 40, 3 });
    CHECK(areas.get("footer") == Rect{ 0, 23, 80, 1 });
}

TEST_CASE("resolution follows the screen Rect")
{
    const layout::Areas areas = layout::resolve(sample_tree(), Rect{ 4, 3, 40, 20 });

    CHECK(areas.get("header") == Rect{ 4, 3, 40, 7 });
    CHECK(areas.get("footer") == Rect{ 4, 22, 40, 1 });
}

TEST_CASE("hit returns the deepest named area containing the point")
{
    const layout::Areas areas = layout::resolve(sample_tree(), Rect{ 0, 0, 80, 24 });

    const std::string* menu = areas.hit({ 5, 10 });
    REQUIRE(menu != nullptr);
    CHECK(*menu == "menu");

    const std::string* input = areas.hit({ 40, 18 });
    REQUIRE(input != nullptr);
    CHECK(*input == "input");  // deeper than the named input-box container

    const std::string* spacer = areas.hit({ 5, 3 });
    REQUIRE(spacer != nullptr);
    CHECK(*spacer == "header");  // unnamed space() row falls back to the parent

    const std::string* padding = areas.hit({ 1, 22 });
    REQUIRE(padding != nullptr);
    CHECK(*padding == "navigation");  // inset padding belongs to the container
}

TEST_CASE("unnamed leaves are not registered")
{
    const layout::Areas areas = layout::resolve(layout::space(), Rect{ 0, 0, 10, 4 });

    CHECK(areas.empty());
    CHECK(areas.hit({ 1, 1 }) == nullptr);
}

TEST_CASE("missing names are reported")
{
    const layout::Areas areas = layout::resolve(sample_tree(), Rect{ 0, 0, 80, 24 });

    CHECK(areas.find("nope") == nullptr);
    CHECK_THROWS_AS(areas.get("nope"), std::out_of_range);
}

TEST_CASE("focusable leaves surface a tab order")
{
    using namespace layout;
    const Areas areas = resolve(
        column({
            fixed(1, leaf("title")),
            flex(focusable("body")),
            fixed(1, focusable(named("status", row({
                flex(focusable("ok")),
                flex(leaf("plain")),
            })))),
        }),
        Rect{ 0, 0, 10, 4 });

    REQUIRE(areas.focus_order().size() == 3);
    CHECK(areas.focus_order()[0] == "body");
    CHECK(areas.focus_order()[1] == "status");  // a container can be a tab stop
    CHECK(areas.focus_order()[2] == "ok");
}
