#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

#include "teacup/layout_tree.hpp"

using namespace teacup;

namespace
{

layout::node sample_tree()
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
    const layout::areas areas = layout::resolve(sample_tree(), rect{ 0, 0, 80, 24 });

    CHECK(areas.get("header") == rect{ 0, 0, 80, 7 });
    CHECK(areas.get("title") == rect{ 2, 2, 62, 1 });
    CHECK(areas.get("spinner") == rect{ 64, 2, 14, 1 });
    CHECK(areas.get("tabs") == rect{ 2, 4, 76, 1 });
    CHECK(areas.get("navigation") == rect{ 0, 7, 33, 16 });
    CHECK(areas.get("menu") == rect{ 2, 9, 29, 6 });
    CHECK(areas.get("players") == rect{ 2, 16, 29, 5 });
    CHECK(areas.get("components") == rect{ 34, 7, 46, 16 });
    CHECK(areas.get("progress") == rect{ 36, 9, 42, 1 });
    CHECK(areas.get("table") == rect{ 36, 11, 42, 4 });
    CHECK(areas.get("input-box") == rect{ 36, 16, 42, 5 });
    CHECK(areas.get("input") == rect{ 37, 17, 40, 3 });
    CHECK(areas.get("footer") == rect{ 0, 23, 80, 1 });
}

TEST_CASE("resolution follows the screen rect")
{
    const layout::areas areas = layout::resolve(sample_tree(), rect{ 4, 3, 40, 20 });

    CHECK(areas.get("header") == rect{ 4, 3, 40, 7 });
    CHECK(areas.get("footer") == rect{ 4, 22, 40, 1 });
}

TEST_CASE("hit returns the deepest named area containing the point")
{
    const layout::areas areas = layout::resolve(sample_tree(), rect{ 0, 0, 80, 24 });

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
    const layout::areas areas = layout::resolve(layout::space(), rect{ 0, 0, 10, 4 });

    CHECK(areas.empty());
    CHECK(areas.hit({ 1, 1 }) == nullptr);
}

TEST_CASE("missing names are reported")
{
    const layout::areas areas = layout::resolve(sample_tree(), rect{ 0, 0, 80, 24 });

    CHECK(areas.find("nope") == nullptr);
    CHECK_THROWS_AS(areas.get("nope"), std::out_of_range);
}
