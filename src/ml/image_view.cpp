#include "simplyml/ml/image_view.hpp"

#include <algorithm>
#include <cmath>

#include <SFML/Graphics/RenderTarget.hpp>
#include <SFML/Graphics/Sprite.hpp>

namespace sml
{

ImageView::ImageView(std::string title, sf::Color accent)
    : Panel{std::move(title), accent}
{}

void ImageView::update(UiContext const& ctx)
{
    Panel::update(ctx);
    if (m_input.version() == m_seen) {
        return;
    }
    m_seen  = m_input.version();
    m_image = m_input.get();
    m_dirty = true;
}

void ImageView::drawContent(sf::RenderTarget& target, UiContext const&)
{
    if (m_image.width == 0 || m_image.height == 0) {
        return;
    }
    if (m_dirty) { // upload on the drawing thread, which owns the GL context
        sf::Vector2u const size{m_image.width, m_image.height};
        if (m_texture.getSize() != size && !m_texture.resize(size)) {
            return;
        }
        m_texture.update(m_image.rgba.data());
        m_texture.setSmooth(false);
        m_dirty = false;
    }
    sf::FloatRect const c     = contentRect();
    float const         scale = std::min(c.size.x / static_cast<float>(m_image.width),
                                         c.size.y / static_cast<float>(m_image.height));
    sf::Vector2f const  drawn{scale * static_cast<float>(m_image.width), scale * static_cast<float>(m_image.height)};
    sf::Sprite sprite{m_texture};
    sprite.setScale({scale, scale});
    sprite.setPosition({std::round(c.position.x + 0.5f * (c.size.x - drawn.x)), std::round(c.position.y + 0.5f * (c.size.y - drawn.y))});
    target.draw(sprite);
}

} // namespace sml
