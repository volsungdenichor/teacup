#include <catch2/catch_test_macros.hpp>

#include "teacup/widgets/tile_map.hpp"

using namespace teacup;

TEST_CASE("tile map renders independent half pixels and a selected bounding box")
{
    const Color upper = Color::hex("#335577");
    const Color lower = Color::hex("#779955");
    widgets::TileMap::View view;
    view.map_size(Size{ 2, 1 }).pixel([&](Point, Point pixel) { return pixel.y % 2 == 0 ? upper : lower; });
    Surface surface{ Size{ 12, 3 } };
    view.render({}, Canvas{ surface });
    CHECK(surface.row_text(0) == "┌────┐▀▀▀▀▀▀");
    CHECK(surface.row_text(1) == "│▀▀▀▀│▀▀▀▀▀▀");
    CHECK(surface.row_text(2) == "└────┘▀▀▀▀▀▀");
    CHECK(surface.at(Point{ 2, 1 }).style.foreground() == upper);
    CHECK(surface.at(Point{ 2, 1 }).style.background() == lower);
}

TEST_CASE("tile map selection moves by whole tiles and scrolls the entire frame into view")
{
    widgets::TileMap::View view;
    view.map_size(Size{ 4, 3 }).viewport_size(Size{ 12, 6 });
    widgets::TileMap::State state;
    CHECK_FALSE(view.update(state, KeyEvent{ Key::left }));
    CHECK(view.update(state, KeyEvent{ Key::right }));
    CHECK(state.selected == Point{ 1, 0 });
    CHECK(state.drawing.offset == Point{});
    CHECK(view.update(state, KeyEvent{ Key::character, "l" }));
    CHECK(state.selected == Point{ 2, 0 });
    CHECK(state.drawing.offset == Point{ 6, 0 });
    CHECK(view.update(state, KeyEvent{ Key::down }));
    CHECK(view.update(state, KeyEvent{ Key::down }));
    CHECK(state.selected == Point{ 2, 2 });
    CHECK(state.drawing.offset == Point{ 6, 3 });
    CHECK_FALSE(view.update(state, KeyEvent{ Key::down }));
    CHECK_FALSE(view.update(state, KeyEvent{ Key::character, "x" }));
}

TEST_CASE("tile map hit testing accounts for scrolling, origin and blank padding")
{
    widgets::TileMap::View view;
    view.map_size(Size{ 4, 3 });
    widgets::TileMap::State state;
    state.drawing.offset = Point{ 7, 2 };
    const Rect area{ 10, 5, 12, 6 };
    CHECK(view.tile_at(state, Point{ 10, 5 }, area) == Point{ 1, 0 });
    CHECK(view.tile_at(state, Point{ 15, 7 }, area) == Point{ 2, 1 });
    CHECK_FALSE(view.tile_at(state, Point{ 9, 5 }, area));
    CHECK(view.update(state, MouseEvent{ Point{ 15, 7 }, MouseButton::left, MouseAction::press }, area));
    CHECK(state.selected == Point{ 2, 1 });
    CHECK_FALSE(view.update(state, MouseEvent{ Point{ 10, 5 }, MouseButton::left, MouseAction::release }, area));
    CHECK(state.selected == Point{ 2, 1 });
    view.map_size(Size{ 1, 1 });
    CHECK_FALSE(view.tile_at(state, Point{ 16, 5 }, area));
}

TEST_CASE("tile map wheel pans without changing selection")
{
    widgets::TileMap::View view;
    view.map_size(Size{ 4, 4 });
    widgets::TileMap::State state;
    const Rect area{ 0, 0, 6, 3 };
    CHECK(view.update(state, MouseEvent{ Point{}, MouseButton::wheel_down }, area));
    CHECK(state.drawing.offset == Point{ 0, 1 });
    CHECK(view.update(
        state, MouseEvent{ Point{}, MouseButton::wheel_down, MouseAction::press, Modifier::shift }, area));
    CHECK(state.drawing.offset == Point{ 1, 1 });
    CHECK(state.selected == Point{});
}

TEST_CASE("tile map clips rendering and only samples visible cells")
{
    widgets::TileMap::View view;
    int calls = 0;
    view.map_size(Size{ 100000, 100000 }).pixel(
        [&](Point tile, Point pixel)
        {
            CHECK(tile == Point{ 1, 1 });
            CHECK(pixel.x >= 1);
            CHECK(pixel.x <= 2);
            ++calls;
            return Color::hex("#336699");
        });
    widgets::TileMap::State state;
    state.drawing.offset = Point{ 7, 4 };
    Surface surface{ Size{ 4, 3 } };
    Canvas canvas{ surface };
    canvas.fill("#");
    view.render(state, canvas.sub(Rect{ 1, 1, 2, 1 }));
    CHECK(calls == 4);
    CHECK(surface.row_text(0) == "####");
    CHECK(surface.row_text(1) == "#▀▀#");
    CHECK(surface.row_text(2) == "####");
}

TEST_CASE("tile map handles empty maps, custom tiles, resize and invalid sizes")
{
    widgets::TileMap::View view;
    widgets::TileMap::State state;
    Surface surface{ Size{ 8, 4 } };
    Canvas canvas{ surface };
    canvas.fill("#");
    view.render(state, canvas);
    CHECK(surface.row_text(0) == "        ");
    CHECK_FALSE(view.update(state, KeyEvent{ Key::right }));
    CHECK_FALSE(view.tile_at(state, Point{}, surface.bounds()));
    CHECK_THROWS_AS(view.tile_size(Size{ 2, 3 }), std::invalid_argument);
    CHECK_THROWS_AS(view.tile_size(Size{ 3, (std::numeric_limits<int>::max)() }), std::invalid_argument);
    CHECK_THROWS_AS(view.map_size(Size{ -1, 3 }), std::invalid_argument);
    CHECK_THROWS_AS(view.map_size(Size{ (std::numeric_limits<int>::max)(), 1 }), std::invalid_argument);
    view.tile_size(Size{ 4, 3 }).map_size(Size{ 3, 2 }).viewport_size(Size{ 4, 3 });
    CHECK_THROWS_AS(view.tile_rect(Point{ -1, 0 }), std::out_of_range);
    CHECK_THROWS_AS(view.tile_rect(Point{ 3, 0 }), std::out_of_range);
    state.selected = Point{ 99, 99 };
    view.ensure_visible(state);
    CHECK(state.selected == Point{ 2, 1 });
    CHECK(state.drawing.offset == Point{ 8, 3 });
    view.viewport_size(Size{ 12, 6 });
    view.ensure_visible(state);
    CHECK(state.drawing.offset == Point{});
    view.render(state, canvas);
    state.selected = Point{};
    view.render(state, canvas);
    CHECK(surface.row_text(0).substr(0, std::string("┌──┐").size()) == "┌──┐");
}
