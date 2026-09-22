#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "teacup/event.hpp"
#include "teacup/layout_tree.hpp"
#include "teacup/message.hpp"
#include "teacup/style.hpp"

namespace teacup
{

using layout::Areas;

class WidgetTraits
{
public:
    using Query = std::function<bool(const std::string& id)>;

    WidgetTraits& enabled(Query fn)
    {
        enabled_ = std::move(fn);
        return *this;
    }
    WidgetTraits& focusable(Query fn)
    {
        focusable_ = std::move(fn);
        return *this;
    }

    [[nodiscard]] bool is_enabled(const std::string& id) const { return !enabled_ || enabled_(id); }
    [[nodiscard]] bool is_focusable(const std::string& id) const
    {
        return is_enabled(id) && (!focusable_ || focusable_(id));
    }

private:
    Query enabled_;
    Query focusable_;
};

class FocusRing
{
public:
    FocusRing() = default;
    explicit FocusRing(std::vector<std::string> order) : order_(std::move(order)) { clamp(); }

    [[nodiscard]] const std::vector<std::string>& order() const { return order_; }

    FocusRing& traits(WidgetTraits value)
    {
        traits_ = std::move(value);
        clamp();
        return *this;
    }

    [[nodiscard]] const std::string& focused() const
    {
        return index_ != npos && index_ < order_.size() && traits_.is_focusable(order_[index_]) ? order_[index_] : empty_;
    }

    [[nodiscard]] bool is(const std::string& id) const { return !focused().empty() && focused() == id; }

    bool set(const std::string& id)
    {
        const auto it = std::find(order_.begin(), order_.end(), id);
        if (it == order_.end() || !traits_.is_focusable(*it))
        {
            return false;
        }
        index_ = static_cast<std::size_t>(std::distance(order_.begin(), it));
        return true;
    }

    void advance(int step = 1)
    {
        if (order_.empty())
        {
            return;
        }
        const int count = static_cast<int>(order_.size());
        for (int tries = 0; tries < count; ++tries)
        {
            index_ = static_cast<std::size_t>((static_cast<int>(index_) + step % count + count) % count);
            if (traits_.is_focusable(order_[index_]))
            {
                return;
            }
        }
        index_ = npos;  // nothing is focusable
    }

private:
    void clamp()
    {
        if (index_ == npos)
        {
            return;
        }
        if (index_ >= order_.size())
        {
            index_ = 0;
        }
        if (!order_.empty() && !traits_.is_focusable(order_[index_]))
        {
            advance();
        }
    }

    static constexpr std::size_t npos = static_cast<std::size_t>(-1);

    inline static const std::string empty_{};

    std::vector<std::string> order_;
    std::size_t index_ = 0;
    WidgetTraits traits_;
};

class Router
{
public:
    using Handler = std::function<bool(const Message&, const Areas&)>;

    Router& add(std::string id, Handler fn)
    {
        handlers_.push_back(Entry{ std::move(id), std::move(fn) });
        return *this;
    }

    Router& clear()
    {
        handlers_.clear();
        return *this;
    }

    Router& focus_order(std::vector<std::string> order, WidgetTraits traits = {})
    {
        focus_ = FocusRing(std::move(order));
        focus_.traits(std::move(traits));
        explicit_order_ = true;
        return *this;
    }

    [[nodiscard]] FocusRing& focus() { return focus_; }
    [[nodiscard]] const FocusRing& focus() const { return focus_; }

    bool route(const Message& msg, const Areas& layout, bool cycle_tab = true)
    {
        if (!explicit_order_ && focus_.order().empty() && !layout.focus_order().empty())
        {
            focus_ = FocusRing(layout.focus_order());
        }
        if (const auto* key = msg_cast<KeyEvent>(msg))
        {
            if (cycle_tab && key->code == Key::tab)
            {
                focus_.advance(has(key->modifiers, Modifier::shift) ? -1 : 1);
                return true;
            }
            bool consumed = false;
            for (const Entry& e : handlers_)
            {
                if (e.id == focus_.focused())
                {
                    consumed = e.fn(msg, layout) || consumed;
                }
            }
            return consumed;
        }
        if (const auto* mouse = msg_cast<MouseEvent>(msg))
        {
            if (mouse->action == MouseAction::press)
            {
                for (const std::string* id = layout.hit(mouse->position); id != nullptr; id = layout.parent(*id))
                {
                    if (has_handler(*id) && focus_.set(*id))
                    {
                        break;
                    }
                }
            }
        }
        bool consumed = false;
        for (const Entry& e : handlers_)
        {
            consumed = e.fn(msg, layout) || consumed;
        }
        return consumed;
    }

private:
    struct Entry
    {
        std::string id;
        Handler fn;
    };

    [[nodiscard]] bool has_handler(const std::string& id) const
    {
        for (const Entry& e : handlers_)
        {
            if (e.id == id)
            {
                return true;
            }
        }
        return false;
    }

    std::vector<Entry> handlers_;
    FocusRing focus_;
    bool explicit_order_ = false;
};

template <class State, class View, class OnConsumed>
Router::Handler route_keys(State& state, const View& view, bool enabled, OnConsumed on_consumed)
{
    return [&state, &view, enabled, on_consumed](const Message& msg, const Areas&)
    {
        if (!enabled)
        {
            return false;
        }
        if (const auto* key = msg_cast<KeyEvent>(msg))
        {
            if (view.update(state, *key))
            {
                on_consumed(*key);
                return true;
            }
        }
        return false;
    };
}

template <class State, class View>
Router::Handler route_keys(State& state, const View& view, bool enabled = true)
{
    return route_keys(state, view, enabled, [](const KeyEvent&) {});
}

template <class State, class View, class OnConsumed>
Router::Handler route_mouse(State& state, const View& view, std::string area_id, bool enabled, OnConsumed on_consumed)
{
    return [&state, &view, area_id = std::move(area_id), enabled, on_consumed](const Message& msg, const Areas& layout)
    {
        if (!enabled)
        {
            return false;
        }
        if (const auto* mouse = msg_cast<MouseEvent>(msg))
        {
            const Rect* area = layout.find(area_id);
            if (area != nullptr && view.update(state, *mouse, *area))
            {
                on_consumed(*mouse);
                return true;
            }
        }
        return false;
    };
}

template <class State, class View>
Router::Handler route_mouse(State& state, const View& view, std::string area_id, bool enabled = true)
{
    return route_mouse(state, view, std::move(area_id), enabled, [](const MouseEvent&) {});
}

[[nodiscard]] inline Style disabled_overlay()
{
    return Style{}.fg(palette::subtle()).dim();
}

}  // namespace teacup
