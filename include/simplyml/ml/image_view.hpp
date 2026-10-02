#pragma once
#include <cstdint>

#include <SFML/Graphics/Texture.hpp>

#include "simplyml/core/snapshot.hpp"
#include "simplyml/ml/formats.hpp"
#include "simplyml/ui/panel.hpp"

namespace sml
{

/// Panel showing an RgbaImage scaled to fit (aspect kept, centered), pixels drawn as sharp squares.
class ImageView : public Panel
{
public:
    explicit ImageView(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    /// Thread-safe: call from the training thread; drawn from the next frame.
    void setImage(RgbaImage image) { m_input.set(std::move(image)); }

    void update(UiContext const& ctx) override;

protected:
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    Snapshot<RgbaImage> m_input;
    std::uint64_t       m_seen = 0;
    RgbaImage           m_image;
    bool                m_dirty = false; // m_image not yet uploaded to m_texture
    sf::Texture         m_texture;
};

} // namespace sml
