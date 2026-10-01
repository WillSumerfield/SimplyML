#pragma once
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Window/Event.hpp>

#include "simplyml/core/metric_store.hpp"
#include "simplyml/ui/theme.hpp"
#include "simplyml/util/smooth_value.hpp"

namespace sml
{

/// What widgets get each frame: the look, the data and the time. UI thread only.
struct UiContext
{
    Theme const&       theme;
    sf::Font const&    font;
    MetricStore const& store; // synced series
    double             now = 0.0; // wall time, seconds
    float              dt  = 0.0f;
    sf::Vector2f       mouse;
};

/// Space a widget asks for along its container's main axis: fixed pixels, a share of what is left
/// after fixed sizes (`fr`), or both (`px` minimum plus a share).
struct Length
{
    float px = 0.0f;
    float fr = 0.0f;
};

[[nodiscard]] constexpr Length px(float v) { return {v, 0.0f}; }
[[nodiscard]] constexpr Length fr(float v) { return {0.0f, v}; }

/// Base of everything on screen. A widget owns a rectangle in screen pixels, draws itself into it
/// and may react to input. Containers place children; leaves draw.
class Widget
{
public:
    virtual ~Widget() = default;

    Widget& setId(std::string id) { m_id = std::move(id); return *this; }
    [[nodiscard]] std::string const& id() const { return m_id; }

    /// Main-axis size inside a Row/Column (default `fr(1)`).
    Widget& setExtent(Length extent) { m_extent = extent; return *this; }
    [[nodiscard]] Length extent() const { return m_extent; }
    /// Cells covered inside a Grid (default 1x1).
    Widget& setSpan(int columns, int rows = 1);
    [[nodiscard]] sf::Vector2i span() const { return m_span; }

    Widget& setVisible(bool visible) { m_visible = visible; return *this; }
    [[nodiscard]] bool visible() const { return m_visible; }

    [[nodiscard]] sf::FloatRect bounds() const { return m_bounds; }
    [[nodiscard]] bool          contains(sf::Vector2f p) const { return m_bounds.contains(p); }

    /// Assigns the widget's rectangle. Calls `onLayout` when it changed (or `invalidate` was called).
    void layout(sf::FloatRect bounds, UiContext const& ctx);
    /// Forces `onLayout` on the next `layout`, e.g. after a property change that moves content.
    void invalidate() { m_dirty = true; }

    /// Per-frame data pull and animation, before drawing.
    virtual void update(UiContext const&) {}
    virtual void draw(sf::RenderTarget& target, UiContext const& ctx) = 0;

    /// Input in screen pixels. Tracks hover/press/focus; returns true to consume the event.
    /// Only widgets that are `interactive()` consume presses and become focused.
    virtual bool handle(sf::Event const& event, UiContext const& ctx);

    /// This widget or a descendant with `id`, else nullptr.
    [[nodiscard]] virtual Widget* find(std::string_view id);

    [[nodiscard]] bool hovered() const { return m_hovered; }
    [[nodiscard]] bool pressed() const { return m_pressed; }
    [[nodiscard]] bool focused() const { return m_focused; }
    /// Smoothed 0..1 hover and press amounts for animation.
    [[nodiscard]] float hoverAmount(double now) const { return m_hoverAnim.get(now); }
    [[nodiscard]] float pressAmount(double now) const { return m_pressAnim.get(now); }

protected:
    virtual void onLayout(UiContext const&) {}
    /// True to run `onLayout` on every `layout` call, changed or not.
    [[nodiscard]] virtual bool alwaysLayout() const { return false; }
    [[nodiscard]] virtual bool interactive() const { return false; }
    /// Left press and release both inside the widget.
    virtual void onClick(UiContext const&) {}
    virtual void onFocusChanged(bool, UiContext const&) {}

private:
    std::string   m_id;
    Length        m_extent = fr(1.0f);
    sf::Vector2i  m_span   = {1, 1};
    sf::FloatRect m_bounds;
    bool          m_visible = true;
    bool          m_dirty   = true;

    bool              m_hovered = false;
    bool              m_pressed = false;
    bool              m_focused = false;
    SmoothValue<float> m_hoverAnim{0.0f, 0.12f, Ease::OutCubic};
    SmoothValue<float> m_pressAnim{0.0f, 0.08f, Ease::OutCubic};
};

/// Widget holding child widgets; draws, updates and routes input to them.
class Container : public Widget
{
public:
    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
        auto w = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *w;
        m_children.push_back(std::move(w));
        invalidate();
        return ref;
    }
    Widget& add(std::unique_ptr<Widget> w);
    /// Removes and destroys `child`; false if it isn't a direct child.
    bool remove(Widget const& child);
    void clear();

    [[nodiscard]] std::vector<std::unique_ptr<Widget>> const& children() const { return m_children; }

    void    update(UiContext const& ctx) override;
    void    draw(sf::RenderTarget& target, UiContext const& ctx) override;
    bool    handle(sf::Event const& event, UiContext const& ctx) override;
    Widget* find(std::string_view id) override;

protected:
    /// Containers re-place children every frame (cheap); unchanged children skip their `onLayout`.
    void onLayout(UiContext const& ctx) override { arrange(ctx); }
    [[nodiscard]] bool alwaysLayout() const override { return true; }
    virtual void arrange(UiContext const& ctx) = 0;

    std::vector<std::unique_ptr<Widget>> m_children;
};

} // namespace sml
