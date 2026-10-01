#include "simplyml/ui/widget.hpp"

#include <algorithm>

namespace sml
{

Widget& Widget::setSpan(int columns, int rows)
{
    m_span = {std::max(columns, 1), std::max(rows, 1)};
    return *this;
}

void Widget::layout(sf::FloatRect bounds, UiContext const& ctx)
{
    if (m_dirty || alwaysLayout() || bounds != m_bounds) {
        m_bounds = bounds;
        m_dirty  = false;
        onLayout(ctx);
    }
}

bool Widget::handle(sf::Event const& event, UiContext const& ctx)
{
    auto setHover = [&](bool h) {
        if (h != m_hovered) {
            m_hovered = h;
            m_hoverAnim.set(h ? 1.0f : 0.0f, ctx.now);
        }
    };
    auto setPressed = [&](bool p) {
        m_pressed = p;
        m_pressAnim.set(p ? 1.0f : 0.0f, ctx.now);
    };

    if (auto const* m = event.getIf<sf::Event::MouseMoved>()) {
        setHover(m_visible && contains(sf::Vector2f(m->position)));
        return false;
    }
    if (event.is<sf::Event::MouseLeft>()) {
        setHover(false);
        return false;
    }
    if (!m_visible || !interactive()) {
        return false;
    }
    if (auto const* p = event.getIf<sf::Event::MouseButtonPressed>()) {
        bool const inside = contains(sf::Vector2f(p->position));
        if (inside && p->button == sf::Mouse::Button::Left) {
            setPressed(true);
            if (!m_focused) {
                m_focused = true;
                onFocusChanged(true, ctx);
            }
            return true;
        }
        if (!inside && m_focused) {
            m_focused = false;
            onFocusChanged(false, ctx);
        }
        return false;
    }
    if (auto const* r = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (m_pressed && r->button == sf::Mouse::Button::Left) {
            setPressed(false);
            if (contains(sf::Vector2f(r->position))) {
                onClick(ctx);
            }
            return true;
        }
    }
    return false;
}

void Widget::setFocused(bool focused, UiContext const& ctx)
{
    if (focused != m_focused) {
        m_focused = focused;
        onFocusChanged(focused, ctx);
    }
}

Widget* Widget::find(std::string_view id)
{
    return !id.empty() && m_id == id ? this : nullptr;
}

Widget& Container::add(std::unique_ptr<Widget> w)
{
    Widget& ref = *w;
    m_children.push_back(std::move(w));
    invalidate();
    return ref;
}

bool Container::remove(Widget const& child)
{
    auto it = std::find_if(m_children.begin(), m_children.end(), [&](auto const& c) { return c.get() == &child; });
    if (it == m_children.end()) {
        return false;
    }
    m_children.erase(it);
    invalidate();
    return true;
}

void Container::clear()
{
    m_children.clear();
    invalidate();
}

void Container::update(UiContext const& ctx)
{
    for (auto& c : m_children) {
        if (c->visible()) {
            c->update(ctx);
        }
    }
}

void Container::draw(sf::RenderTarget& target, UiContext const& ctx)
{
    for (auto& c : m_children) {
        if (c->visible()) {
            c->draw(target, ctx);
        }
    }
}

bool Container::handle(sf::Event const& event, UiContext const& ctx)
{
    // Every child sees every event (hover and focus need it); children don't overlap, so at most
    // one of them claims a press.
    bool consumed = false;
    for (auto& c : m_children) {
        consumed = c->handle(event, ctx) || consumed;
    }
    return consumed;
}

Widget* Container::find(std::string_view id)
{
    if (Widget* w = Widget::find(id)) {
        return w;
    }
    for (auto& c : m_children) {
        if (Widget* w = c->find(id)) {
            return w;
        }
    }
    return nullptr;
}

} // namespace sml
