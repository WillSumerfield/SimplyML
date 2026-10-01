#pragma once
#include <cstdint>
#include <vector>

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Transform.hpp>
#include <SFML/Graphics/VertexArray.hpp>

#include "simplyml/core/snapshot.hpp"
#include "simplyml/ml/formats.hpp"
#include "simplyml/ui/panel.hpp"
#include "simplyml/ui/ruler.hpp"

namespace sml
{

/// Panel drawing a cart-pendulum scene in world units, fitted into the panel: a rail with a scale
/// below it, a wheeled cart at the chain's base, the links and their joints, and an arrow for a
/// disturbance push. Extra "ghost" chains (e.g. the rest of a population) are drawn faded behind
/// the main one. Sizes (link width, joint radius, cart) are in world units, as in the original
/// 1000-unit-wide scene.
class CartPendulumView : public Panel
{
public:
    explicit CartPendulumView(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    /// Thread-safe: call from the simulation thread; drawn from the next frame.
    void setState(LinkChainState state) { m_state.set(std::move(state)); }
    void setGhosts(std::vector<LinkChainState> ghosts) { m_ghosts.set(std::move(ghosts)); }

    /// World area shown (fitted, aspect kept). Default: around the rail.
    CartPendulumView& setWorld(sf::FloatRect world) { m_world = world; m_worldSet = true; invalidate(); return *this; }
    /// Range the cart's base travels over and the rail's height. The drawn rail extends `overhang`
    /// past each end.
    CartPendulumView& setRail(float from, float to, float y, float overhang = 50.0f);
    /// Scale below the rail, labelled relative to `origin` (default: the rail's center).
    CartPendulumView& setRuler(bool show, float halfWidth = 300.0f, float step = 10.0f, int majorEvery = 10);
    CartPendulumView& setRulerOrigin(float x) { m_rulerOrigin = x; m_originSet = true; return *this; }
    /// Wheel image (white on transparent, square); none draws plain spoked wheels. Kept by pointer.
    CartPendulumView& setWheelTexture(sf::Texture const* texture) { m_wheel = texture; return *this; }
    /// Joint colors from the base outward (cycled). Default: green, orange, accent.
    CartPendulumView& setJointColors(std::vector<sf::Color> colors) { m_jointColors = std::move(colors); return *this; }
    CartPendulumView& setGhostAlpha(std::uint8_t alpha) { m_ghostAlpha = alpha; return *this; }
    /// Arrow length per unit of push (default 6.6, the original's).
    CartPendulumView& setPushScale(float scale) { m_pushScale = scale; return *this; }

    /// World -> screen px, after layout (UI thread).
    [[nodiscard]] sf::Transform const& worldTransform() const { return m_toScreen; }
    [[nodiscard]] sf::FloatRect world() const;

    void update(UiContext const& ctx) override;

protected:
    void onLayout(UiContext const& ctx) override;
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    void drawChain(sf::VertexArray& va, LinkChainState const& s, std::uint8_t alpha, Palette const& pal) const;

    Snapshot<LinkChainState>              m_state;
    Snapshot<std::vector<LinkChainState>> m_ghosts;
    LinkChainState const*                 m_current = nullptr; // into the snapshots, until the next update
    std::vector<LinkChainState> const*    m_currentGhosts = nullptr;

    sf::FloatRect m_world;
    bool          m_worldSet = false;
    float         m_railFrom = -250.0f, m_railTo = 250.0f, m_railY = 0.0f, m_overhang = 50.0f;
    bool          m_ruler = true;
    float         m_rulerHalf = 300.0f, m_rulerStep = 10.0f;
    int           m_rulerMajor = 10;
    float         m_rulerOrigin = 0.0f;
    bool          m_originSet = false;

    sf::Texture const*     m_wheel = nullptr;
    std::vector<sf::Color> m_jointColors;
    std::uint8_t           m_ghostAlpha = 50;
    float                  m_pushScale  = 6.6f;

    sf::Transform m_toScreen;
    float         m_zoom = 1.0f;
};

} // namespace sml
