#pragma once

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "teacup/geometry.hpp"
#include "teacup/layout.hpp"

namespace teacup
{
/// Declarative, named layout trees.
///
/// A `layout::node` tree is declared once and resolved against a screen rect,
/// producing `areas`: a flat id → rect map with `hit(point)` for mouse
/// routing. `update` and `view` share the same tree, so event dispatch and
/// painting can never drift apart. Resolution is pure integer math — the
/// tree holds no canvas, widget state, or focus.
namespace layout
{

struct child;

/// One node of a layout tree. Prefer the builder functions below over filling
/// the fields directly.
struct node
{
    enum class kind
    {
        leaf,    ///< terminal node
        row,     ///< children side by side (horizontal split)
        column,  ///< children stacked (vertical split)
        inset    ///< single child shrunk by `margin` on every side
    };

    kind type = kind::leaf;
    std::string id;               ///< when non-empty, the node's rect is registered in `areas`
    std::vector<child> children;  ///< row/column children; inset uses the first child
    int gap = 0;                  ///< row/column: cells between children
    int margin = 0;               ///< inset: cells shaved off each side
};

/// A child slot of a row/column: how much space it takes and what fills it.
struct child
{
    constraint size;
    node content;
};

/// A terminal area registered under `id` after resolution.
inline node leaf(std::string id)
{
    node n;
    n.id = std::move(id);
    return n;
}

/// An unnamed leaf: occupies space without registering a rect.
inline node space()
{
    return {};
}

/// Registers `content`'s rect under `id`, even when it is a container.
inline node named(std::string id, node content)
{
    content.id = std::move(id);
    return content;
}

/// Shrinks `content` by `margin` cells on every side (e.g. a block's border + padding).
inline node inset(int margin, node content)
{
    node n;
    n.type = node::kind::inset;
    n.margin = margin;
    n.children.push_back(child{ constraint::flex(), std::move(content) });  // sizing is unused
    return n;
}

/// Lays out children side by side.
inline node row(std::initializer_list<child> children, int gap = 0)
{
    node n;
    n.type = node::kind::row;
    n.children = children;
    n.gap = gap;
    return n;
}

/// Lays out children top to bottom.
inline node column(std::initializer_list<child> children, int gap = 0)
{
    node n;
    n.type = node::kind::column;
    n.children = children;
    n.gap = gap;
    return n;
}

inline child fixed(int cells, node content)
{
    return { constraint::fixed(cells), std::move(content) };
}

inline child percent(int value, node content)
{
    return { constraint::percent(value), std::move(content) };
}

inline child flex(node content, int weight = 1)
{
    return { constraint::flex(weight), std::move(content) };
}

inline child min(int cells, node content)
{
    return { constraint::min(cells), std::move(content) };
}

/// The result of resolving a layout tree: named rects plus hit-testing.
class areas
{
public:
    using value_type = std::pair<std::string, rect>;
    using const_iterator = std::vector<value_type>::const_iterator;

    /// The rect registered under `id`, or nullptr when there is none.
    [[nodiscard]] const rect* find(const std::string& id) const
    {
        for (const value_type& entry : entries_)
        {
            if (entry.first == id)
            {
                return &entry.second;
            }
        }
        return nullptr;
    }

    /// The rect registered under `id`; throws std::out_of_range when absent.
    [[nodiscard]] rect get(const std::string& id) const
    {
        if (const rect* r = find(id))
        {
            return *r;
        }
        throw std::out_of_range("teacup layout: no area named '" + id + "'");
    }

    /// The deepest registered area containing `p`, or nullptr.
    ///
    /// Containers are registered before their children and siblings never
    /// overlap, so the last match in declaration order is the most specific.
    [[nodiscard]] const std::string* hit(point p) const
    {
        const std::string* best = nullptr;
        for (const value_type& entry : entries_)
        {
            if (entry.second.contains(p))
            {
                best = &entry.first;
            }
        }
        return best;
    }

    /// The nearest named ancestor of `id`, or nullptr at the root. Lets
    /// callers walk from a hit leaf up to the widget that owns the area.
    [[nodiscard]] const std::string* parent(const std::string& id) const
    {
        for (const auto& entry : parents_)
        {
            if (entry.first == id)
            {
                return entry.second.empty() ? nullptr : &entry.second;
            }
        }
        return nullptr;
    }

    [[nodiscard]] std::size_t size() const
    {
        return entries_.size();
    }
    [[nodiscard]] bool empty() const
    {
        return entries_.empty();
    }

    [[nodiscard]] const_iterator begin() const
    {
        return entries_.begin();
    }
    [[nodiscard]] const_iterator end() const
    {
        return entries_.end();
    }

private:
    friend areas resolve(const node& root, const rect& screen);

    static void resolve_node(const node& n, const rect& area, areas& out, const std::string& enclosing);

    std::vector<value_type> entries_;
    std::vector<std::pair<std::string, std::string>> parents_;  // id → enclosing named id ("" at the root)
};

/// Resolves `root` against `screen`, in declaration (pre-order) order.
inline areas resolve(const node& root, const rect& screen)
{
    areas result;
    areas::resolve_node(root, screen, result, "");
    return result;
}

inline void areas::resolve_node(const node& n, const rect& area, areas& out, const std::string& enclosing)
{
    std::string innermost = enclosing;
    if (!n.id.empty())
    {
        out.entries_.emplace_back(n.id, area);
        out.parents_.emplace_back(n.id, enclosing);
        innermost = n.id;
    }
    switch (n.type)
    {
        case node::kind::leaf:
            break;
        case node::kind::inset:
            if (!n.children.empty())
            {
                resolve_node(n.children.front().content, area.inset(n.margin), out, innermost);
            }
            break;
        case node::kind::row:
        case node::kind::column:
        {
            std::vector<constraint> constraints;
            constraints.reserve(n.children.size());
            for (const child& c : n.children)
            {
                constraints.push_back(c.size);
            }
            const std::vector<rect> rects = n.type == node::kind::row
                ? split_horizontal(area, constraints, n.gap)
                : split_vertical(area, constraints, n.gap);
            for (std::size_t i = 0; i < n.children.size(); ++i)
            {
                resolve_node(n.children[i].content, rects[i], out, innermost);
            }
            break;
        }
    }
}

}  // namespace layout
}  // namespace teacup
