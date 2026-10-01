#pragma once
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/Texture.hpp>

namespace sml
{

struct ResourceError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

/// Named fonts and textures owned by one App. Always holds the embedded default font.
/// Loading failures and unknown names throw `ResourceError`; `find*` returns nullptr instead.
/// Not thread-safe: use from the UI thread, or before the App starts.
class Resources
{
public:
    Resources();

    [[nodiscard]] sf::Font const& defaultFont() const { return *m_defaultFont; }

    sf::Font&    loadFont(std::string const& name, std::filesystem::path const& path);
    /// `data` must outlive the font (SFML reads it lazily).
    sf::Font&    loadFontFromMemory(std::string const& name, void const* data, std::size_t size);
    sf::Texture& loadTexture(std::string const& name, std::filesystem::path const& path, bool smooth = true);
    sf::Texture& addTexture(std::string const& name, sf::Texture texture);

    [[nodiscard]] sf::Font const&    font(std::string_view name) const;
    [[nodiscard]] sf::Texture const& texture(std::string_view name) const;
    [[nodiscard]] sf::Font const*    findFont(std::string_view name) const;
    [[nodiscard]] sf::Texture const* findTexture(std::string_view name) const;

private:
    // unique_ptr: references stay valid as entries are added
    std::unique_ptr<sf::Font>                                       m_defaultFont;
    std::map<std::string, std::unique_ptr<sf::Font>, std::less<>>    m_fonts;
    std::map<std::string, std::unique_ptr<sf::Texture>, std::less<>> m_textures;
};

} // namespace sml
