#include <catch2/catch_test_macros.hpp>

#include "teacup/detail/input_decoder.hpp"
#include "teacup/event.hpp"

using namespace teacup;

namespace
{

std::vector<Message> decode(const std::string& bytes)
{
    detail::InputDecoder decoder;
    return decoder.feed(bytes);
}

}  // namespace

TEST_CASE("plain characters decode to key events")
{
    const auto events = decode("ab");
    REQUIRE(events.size() == 2);
    CHECK(msg_cast<KeyEvent>(events[0])->name() == "a");
    CHECK(msg_cast<KeyEvent>(events[1])->name() == "b");
}

TEST_CASE("control characters carry the ctrl modifier")
{
    const auto events = decode("\x03");
    REQUIRE(events.size() == 1);
    CHECK(msg_cast<KeyEvent>(events[0])->name() == "ctrl+c");
}

TEST_CASE("csi sequences decode to navigation keys")
{
    const auto events = decode("\x1b[A\x1b[6~\x1b[1;5C");
    REQUIRE(events.size() == 3);
    CHECK(msg_cast<KeyEvent>(events[0])->name() == "up");
    CHECK(msg_cast<KeyEvent>(events[1])->name() == "pgdown");
    CHECK(msg_cast<KeyEvent>(events[2])->name() == "ctrl+right");
}

TEST_CASE("sequences split across chunks are buffered")
{
    detail::InputDecoder decoder;
    CHECK(decoder.feed("\x1b[").empty());
    const auto events = decoder.feed("B");
    REQUIRE(events.size() == 1);
    CHECK(msg_cast<KeyEvent>(events[0])->code == Key::down);
}

TEST_CASE("a lone escape is resolved on flush")
{
    detail::InputDecoder decoder;
    CHECK(decoder.feed("\x1b").empty());
    const auto events = decoder.flush();
    REQUIRE(events.size() == 1);
    CHECK(msg_cast<KeyEvent>(events[0])->code == Key::escape);
}

TEST_CASE("sgr mouse reports decode to mouse events")
{
    const auto events = decode("\x1b[<0;10;5M");
    REQUIRE(events.size() == 1);
    const auto* ev = msg_cast<MouseEvent>(events[0]);
    REQUIRE(ev != nullptr);
    CHECK(ev->position == Point{ 9, 4 });
    CHECK(ev->button == MouseButton::left);
    CHECK(ev->action == MouseAction::press);
}

TEST_CASE("bracketed paste is collected into a single event")
{
    const auto events = decode("\x1b[200~hello\x1b[201~");
    REQUIRE(events.size() == 1);
    REQUIRE(msg_cast<PasteEvent>(events[0]) != nullptr);
    CHECK(msg_cast<PasteEvent>(events[0])->text == "hello");
}
