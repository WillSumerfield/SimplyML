#include "simplyml/core/resources.hpp"

#include "simplyml/core/default_font.hpp"

namespace sml
{

namespace
{

template<typename Map>
auto* findIn(Map const& map, std::string_view name)
{
    auto const it = map.find(name);
    return it == map.end() ? nullptr : it->second.get();
}

} // namespace

Resources::Resources()
    : m_defaultFont{std::make_unique<sf::Font>()}
{
    auto const bytes = defaultFontData();
    if (!m_defaultFont->openFromMemory(bytes.data, bytes.size)) {
        throw ResourceError("SimplyML: failed to load embedded default font");
    }
}

sf::Font& Resources::loadFont(std::string const& name, std::filesystem::path const& path)
{
    auto font = std::make_unique<sf::Font>();
    if (!font->openFromFile(path)) {
        throw ResourceError("SimplyML: failed to load font '" + name + "' from '" + path.string() + "'");
    }
    return *(m_fonts[name] = std::move(font));
}

sf::Font& Resources::loadFontFromMemory(std::string const& name, void const* data, std::size_t size)
{
    auto font = std::make_unique<sf::Font>();
    if (!font->openFromMemory(data, size)) {
        throw ResourceError("SimplyML: failed to load font '" + name + "' from memory");
    }
    return *(m_fonts[name] = std::move(font));
}

sf::Texture& Resources::loadTexture(std::string const& name, std::filesystem::path const& path, bool smooth)
{
    auto tex = std::make_unique<sf::Texture>();
    if (!tex->loadFromFile(path)) {
        throw ResourceError("SimplyML: failed to load texture '" + name + "' from '" + path.string() + "'");
    }
    tex->setSmooth(smooth);
    return *(m_textures[name] = std::move(tex));
}

sf::Texture& Resources::addTexture(std::string const& name, sf::Texture texture)
{
    return *(m_textures[name] = std::make_unique<sf::Texture>(std::move(texture)));
}

sf::Font const* Resources::findFont(std::string_view name) const
{
    return findIn(m_fonts, name);
}

sf::Texture const* Resources::findTexture(std::string_view name) const
{
    return findIn(m_textures, name);
}

sf::Font const& Resources::font(std::string_view name) const
{
    if (auto const* f = findFont(name)) {
        return *f;
    }
    throw ResourceError("SimplyML: unknown font '" + std::string(name) + "'");
}

sf::Texture const& Resources::texture(std::string_view name) const
{
    if (auto const* t = findTexture(name)) {
        return *t;
    }
    throw ResourceError("SimplyML: unknown texture '" + std::string(name) + "'");
}

} // namespace sml
