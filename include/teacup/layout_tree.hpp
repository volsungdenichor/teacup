#pragma once

#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "teacup/geometry.hpp"
#include "teacup/layout.hpp"

namespace teacup
{
namespace layout
{

struct Child;

struct Node
{
    enum class Kind
    {
        leaf,    ///< terminal node
        row,     ///< children side by side (horizontal split)
        column,  ///< children stacked (vertical split)
        inset,   ///< single child shrunk by `margin` on every side
        within   ///< single child laid out in a container's content rectangle
    };

    Kind type = Kind::leaf;
    std::string id;               ///< when non-empty, the node's rect is registered in `areas`
    bool focusable = false;       ///< when true, the id joins the tab order in `areas::focus_order`
    std::vector<Child> children;  ///< row/column children; inset/within use the first child
    int gap = 0;                  ///< row/column: cells between children
    int margin = 0;               ///< inset: cells shaved off each side
    std::function<Rect(const Rect&)> content_area;
};

struct Child
{
    Constraint size;
    Node content;
};

inline Node leaf(std::string id)
{
    Node n;
    n.id = std::move(id);
    return n;
}

inline Node focusable(std::string id)
{
    Node n = leaf(std::move(id));
    n.focusable = true;
    return n;
}

inline Node focusable(Node content)
{
    content.focusable = true;
    return content;
}

inline Node space()
{
    return {};
}

inline Node named(std::string id, Node content)
{
    content.id = std::move(id);
    return content;
}

inline Node inset(int margin, Node content)
{
    Node n;
    n.type = Node::Kind::inset;
    n.margin = margin;
    n.children.push_back(Child{ Constraint::flex(), std::move(content) });  // sizing is unused
    return n;
}

/// Resolves children inside a pure surface-space content rectangle, preserving its clip.
inline Node within(std::function<Rect(const Rect&)> content_area, Node content)
{
    if (!content_area)
    {
        throw std::invalid_argument("teacup layout: content area function is empty");
    }
    Node n;
    n.type = Node::Kind::within;
    n.content_area = std::move(content_area);
    n.children.push_back(Child{ Constraint::flex(), std::move(content) });
    return n;
}

inline Node row(std::initializer_list<Child> children, int gap = 0)
{
    Node n;
    n.type = Node::Kind::row;
    n.children = children;
    n.gap = gap;
    return n;
}

inline Node column(std::initializer_list<Child> children, int gap = 0)
{
    Node n;
    n.type = Node::Kind::column;
    n.children = children;
    n.gap = gap;
    return n;
}

inline Child fixed(int cells, Node content)
{
    return { Constraint::fixed(cells), std::move(content) };
}

inline Child percent(int value, Node content)
{
    return { Constraint::percent(value), std::move(content) };
}

inline Child flex(Node content, int weight = 1)
{
    return { Constraint::flex(weight), std::move(content) };
}

inline Child min(int cells, Node content)
{
    return { Constraint::min(cells), std::move(content) };
}

class Areas
{
public:
    using value_type = std::pair<std::string, Rect>;
    using const_iterator = std::vector<value_type>::const_iterator;

    [[nodiscard]] const Rect* find(const std::string& id) const
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

    [[nodiscard]] Rect get(const std::string& id) const
    {
        if (const Rect* r = find(id))
        {
            return *r;
        }
        throw std::out_of_range("teacup layout: no area named '" + id + "'");
    }

    [[nodiscard]] const std::string* hit(Point p) const
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

    [[nodiscard]] const std::vector<std::string>& focus_order() const { return focusable_; }

    [[nodiscard]] std::size_t size() const { return entries_.size(); }
    [[nodiscard]] bool empty() const { return entries_.empty(); }

    [[nodiscard]] const_iterator begin() const { return entries_.begin(); }
    [[nodiscard]] const_iterator end() const { return entries_.end(); }

private:
    friend Areas resolve(const Node& root, const Rect& screen);

    static void resolve_node(
        const Node& n, const Rect& area, Areas& out, const std::string& enclosing, const Rect* clip = nullptr);

    std::vector<value_type> entries_;
    std::vector<std::pair<std::string, std::string>> parents_;  // id → enclosing named id ("" at the root)
    std::vector<std::string> focusable_;                        // focusable ids in declaration order
};

/// Resolves `root` against `screen`, in declaration (pre-order) order.
inline Areas resolve(const Node& root, const Rect& screen)
{
    Areas result;
    Areas::resolve_node(root, screen, result, "");
    return result;
}

inline void Areas::resolve_node(
    const Node& n, const Rect& requested, Areas& out, const std::string& enclosing, const Rect* clip)
{
    const Rect area = clip ? requested.intersect(*clip) : requested;
    std::string innermost = enclosing;
    if (!n.id.empty())
    {
        out.entries_.emplace_back(n.id, area);
        out.parents_.emplace_back(n.id, enclosing);
        innermost = n.id;
    }
    if (n.focusable && !n.id.empty())
    {
        out.focusable_.push_back(n.id);
    }
    switch (n.type)
    {
        case Node::Kind::leaf: break;
        case Node::Kind::inset:
            if (!n.children.empty())
            {
                resolve_node(n.children.front().content, area.inset(n.margin), out, innermost, clip);
            }
            break;
        case Node::Kind::within:
            if (!n.children.empty())
            {
                if (!n.content_area)
                {
                    throw std::invalid_argument("teacup layout: content area function is empty");
                }
                const Rect content = n.content_area(area).intersect(area);
                resolve_node(n.children.front().content, content, out, innermost, &content);
            }
            break;
        case Node::Kind::row:
        case Node::Kind::column:
        {
            std::vector<Constraint> constraints;
            constraints.reserve(n.children.size());
            for (const Child& c : n.children)
            {
                constraints.push_back(c.size);
            }
            const std::vector<Rect> rects = n.type == Node::Kind::row ? split_horizontal(area, constraints, n.gap)
                                                                      : split_vertical(area, constraints, n.gap);
            for (std::size_t i = 0; i < n.children.size(); ++i)
            {
                resolve_node(n.children[i].content, rects[i], out, innermost, clip);
            }
            break;
        }
    }
}

}  // namespace layout
}  // namespace teacup
