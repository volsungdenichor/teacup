#include <catch2/catch_test_macros.hpp>

#include "teacup/surface.hpp"
#include "teacup/widgets/drawing_area.hpp"

using namespace teacup;

namespace
{
// 8x4 content: each row is "rN" followed by letters, so the window is easy to identify.
widgets::DrawingArea::View make_view()
{
    widgets::DrawingArea::View view;
    view.content_size(Size{ 8, 4 })
        .viewport_size(Size{ 4, 2 })
        .draw(
            [](Canvas c)
            {
                c.text(Point{ 0, 0 }, "abcdefgh");
                c.text(Point{ 0, 1 }, "ijklmnop");
                c.text(Point{ 0, 2 }, "qrstuvwx");
                c.text(Point{ 0, 3 }, "yz012345");
            });
    return view;
}
}  // namespace

TEST_CASE("drawing area shows the scrolled window of its content")
{
    Surface s{ Size{ 4, 2 } };
    Canvas c{ s };
    const auto view = make_view();
    widgets::DrawingArea::State state;

    view.render(state, c);
    CHECK(s.row_text(0) == "abcd");
    CHECK(s.row_text(1) == "ijkl");

    state.offset = Point{ 3, 1 };
    view.render(state, c);
    CHECK(s.row_text(0) == "lmno");
    CHECK(s.row_text(1) == "tuvw");

    state.offset = Point{ 99, 99 };
    view.render(state, c);
    CHECK(s.row_text(0) == "uvwx");
    CHECK(s.row_text(1) == "2345");
}

TEST_CASE("drawing area renders into a sub-canvas without touching the rest")
{
    Surface s{ Size{ 8, 3 } };
    s.clear();
    Canvas root{ s };
    root.fill("#");
    const auto view = make_view();
    widgets::DrawingArea::State state{ Point{ 2, 2 } };

    view.render(state, root.sub(Rect{ 2, 1, 4, 2 }));
    CHECK(s.row_text(0) == "########");
    CHECK(s.row_text(1) == "##stuv##");
    CHECK(s.row_text(2) == "##0123##");
}

TEST_CASE("drawing area pads with background when content is smaller than the widget")
{
    Surface s{ Size{ 5, 3 } };
    Canvas c{ s };
    c.fill("#");
    widgets::DrawingArea::View view;
    view.content_size(Size{ 2, 2 }).draw([](Canvas cv) { cv.fill("x"); });
    widgets::DrawingArea::State state{ Point{ 1, 1 } };

    view.render(state, c);
    CHECK(s.row_text(0) == "xx   ");
    CHECK(s.row_text(1) == "xx   ");
    CHECK(s.row_text(2) == "     ");
}

TEST_CASE("drawing area blanks wide glyphs cut by the window edges")
{
    Surface s{ Size{ 3, 1 } };
    Canvas c{ s };
    widgets::DrawingArea::View view;
    view.content_size(Size{ 6, 1 }).draw([](Canvas cv) { cv.text(Point{ 0, 0 }, "a界b界"); });

    view.render(widgets::DrawingArea::State{ Point{ 2, 0 } }, c);
    CHECK(s.row_text(0) == " b ");

    view.render(widgets::DrawingArea::State{ Point{ 3, 0 } }, c);
    CHECK(s.row_text(0) == "b界");
}

TEST_CASE("drawing area scrolls with keys and the mouse wheel")
{
    const auto view = make_view();
    widgets::DrawingArea::State state;

    CHECK_FALSE(view.update(state, KeyEvent{ Key::up }));
    CHECK(view.update(state, KeyEvent{ Key::down }));
    CHECK(state.offset == Point{ 0, 1 });
    CHECK(view.update(state, KeyEvent{ Key::character, "l" }));
    CHECK(state.offset == Point{ 1, 1 });
    CHECK(view.update(state, KeyEvent{ Key::page_down }));
    CHECK(state.offset == Point{ 1, 2 });
    CHECK_FALSE(view.update(state, KeyEvent{ Key::page_down }));
    CHECK(view.update(state, KeyEvent{ Key::home }));
    CHECK(state.offset == Point{ 0, 0 });
    CHECK(view.update(state, KeyEvent{ Key::end }));
    CHECK(state.offset == Point{ 0, 2 });
    CHECK_FALSE(view.update(state, KeyEvent{ Key::character, "x" }));

    const Rect area{ 10, 10, 4, 2 };
    CHECK_FALSE(view.update(state, MouseEvent{ Point{ 0, 0 }, MouseButton::wheel_up }, area));
    CHECK(view.update(state, MouseEvent{ Point{ 11, 11 }, MouseButton::wheel_up }, area));
    CHECK(state.offset == Point{ 0, 1 });
    CHECK(view.update(
        state, MouseEvent{ Point{ 11, 11 }, MouseButton::wheel_down, MouseAction::press, Modifier::shift }, area));
    CHECK(state.offset == Point{ 1, 1 });
}

TEST_CASE("drawing area maps between screen and content coordinates")
{
    const auto view = make_view();
    const widgets::DrawingArea::State state{ Point{ 3, 1 } };
    const Rect area{ 10, 5, 4, 2 };

    CHECK(view.to_content(state, Point{ 10, 5 }, area) == Point{ 3, 1 });
    CHECK(view.to_content(state, Point{ 13, 6 }, area) == Point{ 6, 2 });
    CHECK_FALSE(view.to_content(state, Point{ 14, 6 }, area).has_value());

    CHECK(view.to_screen(state, Point{ 6, 2 }, area) == Point{ 13, 6 });
    CHECK_FALSE(view.to_screen(state, Point{ 0, 0 }, area).has_value());
    CHECK(view.visible_rect(state, area.size()) == Rect{ 3, 1, 4, 2 });
}

TEST_CASE("drawing area ensure_visible scrolls minimally")
{
    const auto view = make_view();
    widgets::DrawingArea::State state;

    view.ensure_visible(state, Rect{ 5, 0, 1, 1 });
    CHECK(state.offset == Point{ 2, 0 });
    view.ensure_visible(state, Rect{ 3, 0, 1, 1 });
    CHECK(state.offset == Point{ 2, 0 });
    view.ensure_visible(state, Rect{ 0, 3, 1, 1 });
    CHECK(state.offset == Point{ 0, 2 });
}
