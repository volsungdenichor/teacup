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

using layout::areas;

/// Reports per-widget capabilities to the focus and routing machinery.
///
/// The two queries are independent: a disabled widget (`enabled == false`)
/// receives no input and is usually painted dimmed, while `focusable` only
/// controls tab-stop membership (e.g. a status label is enabled but never
/// focusable). Disabled implies not focusable.
class widget_traits
{
public:
    using query = std::function<bool(const std::string& id)>;

    widget_traits& enabled(query fn)
    {
        enabled_ = std::move(fn);
        return *this;
    }
    widget_traits& focusable(query fn)
    {
        focusable_ = std::move(fn);
        return *this;
    }

    [[nodiscard]] bool is_enabled(const std::string& id) const
    {
        return !enabled_ || enabled_(id);
    }
    [[nodiscard]] bool is_focusable(const std::string& id) const
    {
        return is_enabled(id) && (!focusable_ || focusable_(id));
    }

private:
    query enabled_;
    query focusable_;
};

/// An ordered set of focusable widget ids, with a current focus.
///
/// Widget ids normally match layout leaf names, so focus, hit-testing and
/// painting all speak the same vocabulary. The ring owns only the order and
/// the focused id — widget state stays in the application model. Entries the
/// traits mark as not focusable are skipped by `advance` and rejected by
/// `set`.
class focus_ring
{
public:
    focus_ring() = default;
    explicit focus_ring(std::vector<std::string> order)
        : order_(std::move(order))
    {
        clamp();
    }

    /// The focusable ids in tab order.
    [[nodiscard]] const std::vector<std::string>& order() const
    {
        return order_;
    }

    /// Capability lookup applied by `advance`/`set`.
    focus_ring& traits(widget_traits value)
    {
        traits_ = std::move(value);
        clamp();
        return *this;
    }

    /// The currently focused id; empty when nothing can be focused.
    [[nodiscard]] const std::string& focused() const
    {
        return index_ != npos && index_ < order_.size() && traits_.is_focusable(order_[index_]) ? order_[index_] : empty_;
    }

    /// True while `id` holds the focus.
    [[nodiscard]] bool is(const std::string& id) const
    {
        return !focused().empty() && focused() == id;
    }

    /// Moves focus to `id`. Returns false when `id` is unknown or not
    /// currently focusable.
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

    /// Advances focus cyclically, skipping entries that are not focusable;
    /// negative steps move backwards. No-op when nothing is focusable.
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
    widget_traits traits_;
};

/// Routes events to widgets by focus and hit-testing.
///
/// Widgets are bound once under the same ids used by the layout tree:
///
/// ```cpp
/// teacup::router events;
/// events.add("menu", teacup::route_keys(m.menu, menu_view, true, [](teacup::key_event) { /* on change */ }));
/// events.add("input", teacup::route_keys(m.input, input_view, m.editing));
/// events.focus_order({ "tabs", "menu", "input" });
/// ```
///
/// - key events go to the focused widget only (after optional tab cycling)
/// - mouse presses focus the hit layout area, then the event is offered to
///   every enabled widget — each one hit-tests internally and ignores misses
/// - disabled widgets receive nothing; paint them with `disabled_overlay()`
class router
{
public:
    /// Callbacks produce side effects such as focus changes or derived state
    /// updates; they run only when the widget consumed the event.
    using handler = std::function<bool(const message&, const areas&)>;

    /// Binds `fn` to `id`. A later `add` for the same id replaces the first.
    router& add(std::string id, handler fn)
    {
        auto it = std::find_if(handlers_.begin(), handlers_.end(),
                               [&](const entry& e) { return e.id == id; });
        if (it != handlers_.end())
        {
            it->fn = std::move(fn);
            return *this;
        }
        handlers_.push_back(entry{ std::move(id), std::move(fn) });
        return *this;
    }

    /// Sets (or replaces) the focus ring used for key routing and tab cycling.
    router& focus_order(std::vector<std::string> order, widget_traits traits = {})
    {
        focus_ = focus_ring(std::move(order));
        focus_.traits(std::move(traits));
        return *this;
    }

    [[nodiscard]] focus_ring& focus()
    {
        return focus_;
    }
    [[nodiscard]] const focus_ring& focus() const
    {
        return focus_;
    }

    /// Routes one event. Returns true when a widget consumed it.
    ///
    /// Key events reach only the focused widget; `key::tab` cycles the focus
    /// first when `cycle_tab` is set. Mouse presses move focus to the hit
    /// area (when that area has a handler and is focusable), then every
    /// widget gets the event and hit-tests internally. Other messages go to
    /// every widget.
    bool route(const message& msg, const areas& layout, bool cycle_tab = true)
    {
        if (const auto* key = msg_cast<key_event>(msg))
        {
            if (cycle_tab && key->code == key::tab)
            {
                focus_.advance(has(key->modifiers, modifier::shift) ? -1 : 1);
                return true;
            }
            if (const handler* fn = find(focus_.focused()))
            {
                return (*fn)(msg, layout);
            }
            return false;
        }
        if (const auto* mouse = msg_cast<mouse_event>(msg))
        {
            if (mouse->action == mouse_action::press)
            {
                // Walk from the deepest hit up through its ancestors and focus
                // the first id that is both bound and focusable.
                for (const std::string* id = layout.hit(mouse->position); id != nullptr; id = layout.parent(*id))
                {
                    if (find(*id) != nullptr && focus_.set(*id))
                    {
                        break;
                    }
                }
            }
        }
        bool consumed = false;
        for (const entry& e : handlers_)
        {
            consumed = e.fn(msg, layout) || consumed;
        }
        return consumed;
    }

private:
    struct entry
    {
        std::string id;
        handler fn;
    };

    [[nodiscard]] const handler* find(const std::string& id) const
    {
        for (const entry& e : handlers_)
        {
            if (e.id == id)
            {
                return &e.fn;
            }
        }
        return nullptr;
    }

    std::vector<entry> handlers_;
    focus_ring focus_;
};

/// Builds a handler that forwards key events to `view.update(state, key)`
/// while `enabled` holds. `on_consumed` runs only when the widget accepted
/// the key.
template <typename State, typename View, typename OnConsumed>
router::handler route_keys(State& state, const View& view, bool enabled, OnConsumed on_consumed)
{
    return [&state, &view, enabled, on_consumed](const message& msg, const areas&)
    {
        if (!enabled)
        {
            return false;
        }
        if (const auto* key = msg_cast<key_event>(msg))
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

template <typename State, typename View>
router::handler route_keys(State& state, const View& view, bool enabled = true)
{
    return route_keys(state, view, enabled, [](const key_event&) {});
}

/// Builds a handler that forwards mouse events to `view.update(state, mouse,
/// rect)` while `enabled` holds, resolving `rect` from the layout under
/// `area_id` (normally the id the handler is bound to).
template <typename State, typename View, typename OnConsumed>
router::handler
route_mouse(State& state, const View& view, std::string area_id, bool enabled, OnConsumed on_consumed)
{
    return [&state, &view, area_id = std::move(area_id), enabled, on_consumed](const message& msg, const areas& layout)
    {
        if (!enabled)
        {
            return false;
        }
        if (const auto* mouse = msg_cast<mouse_event>(msg))
        {
            const rect* area = layout.find(area_id);
            if (area != nullptr && view.update(state, *mouse, *area))
            {
                on_consumed(*mouse);
                return true;
            }
        }
        return false;
    };
}

template <typename State, typename View>
router::handler route_mouse(State& state, const View& view, std::string area_id, bool enabled = true)
{
    return route_mouse(state, view, std::move(area_id), enabled, [](const mouse_event&) {});
}

/// The style applications should merge over a disabled widget's own style:
/// muted and dimmed, regardless of palette.
[[nodiscard]] inline style disabled_overlay()
{
    return style{}.fg(palette::subtle()).dim();
}

}  // namespace teacup
