#pragma once
#include <functional>
#include <string>

#include <SFML/Window/Keyboard.hpp>

#include "simplyml/ui/panel.hpp"

namespace sml
{

/// A hotkey registered with a Ui. It runs `action`, or triggers the control with id `control`;
/// with neither it only documents a key handled elsewhere (and doesn't consume it).
struct KeyBinding
{
    sf::Keyboard::Key     key = sf::Keyboard::Key::Unknown;
    std::string           description;
    std::function<void()> action;
    std::string           control;
};

/// Display form of a key: "S", "Space", "F1", "Up".
[[nodiscard]] std::string keyLabel(sf::Keyboard::Key key);

/// Panel listing the Ui's key bindings as key caps and descriptions, in registration order.
class KeyBindings : public Panel
{
public:
    explicit KeyBindings(std::string title = "Keys", sf::Color accent = sf::Color::Transparent);

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;

protected:
    void drawContent(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    [[nodiscard]] float rowHeight(UiContext const& ctx) const;
};

} // namespace sml
