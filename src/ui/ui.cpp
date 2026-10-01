#include "simplyml/ui/ui.hpp"

#include <algorithm>

#include "simplyml/ui/controls.hpp"

namespace sml
{

Ui::Ui(App& app, Theme theme)
    : m_app{app}
    , m_theme{theme}
    , m_root{std::make_unique<Column>()}
{
    if (!m_theme.font) {
        m_theme.font = &app.resources().defaultFont();
    }
    m_drawId = app.onDraw([this](Canvas& canvas) {
        frame(canvas.target(), m_app.clock().now(), m_app.clock().dt(), m_app.mouseScreen());
    });
    m_eventId = app.events().subscribe([this](sf::Event const& e) { return handle(e); });
}

Ui::~Ui()
{
    m_app.removeDraw(m_drawId);
    m_app.events().unsubscribe(m_eventId);
}

Widget& Ui::operator[](std::string_view id)
{
    Widget* w = find(id);
    if (!w) {
        throw std::out_of_range("SimplyML: no widget with id '" + std::string(id) + "'");
    }
    return *w;
}

UiContext Ui::context(double now, float dt, sf::Vector2f mouse)
{
    return UiContext{m_theme, *m_theme.font, m_app.store(), now, dt, mouse, &m_app.controls(), &m_app.events(), &m_keys};
}

bool Ui::handle(sf::Event const& event)
{
    std::lock_guard lock{m_mutex};
    UiContext const ctx = context(m_app.clock().now(), m_app.clock().dt(), m_app.mouseScreen());
    if (m_root->handle(event, ctx)) {
        return true;
    }
    auto const* k = event.getIf<sf::Event::KeyPressed>();
    if (!k || k->control || k->alt || k->system) {
        return false;
    }
    auto it = std::find_if(m_keys.begin(), m_keys.end(), [&](KeyBinding const& b) { return b.key == k->code; });
    if (it == m_keys.end()) {
        return false;
    }
    if (it->action) {
        auto const action = it->action; // may rebind keys
        action();
        return true;
    }
    if (!it->control.empty()) {
        if (auto* c = dynamic_cast<Control*>(m_root->find(it->control))) {
            c->trigger(ctx);
            return true;
        }
    }
    return false;
}

void Ui::addBinding(KeyBinding binding)
{
    std::lock_guard lock{m_mutex};
    auto it = std::find_if(m_keys.begin(), m_keys.end(), [&](KeyBinding const& b) { return b.key == binding.key; });
    if (it != m_keys.end()) {
        *it = std::move(binding);
    } else {
        m_keys.push_back(std::move(binding));
    }
}

void Ui::bindKey(sf::Keyboard::Key key, std::string description, std::function<void()> action)
{
    addBinding({key, std::move(description), std::move(action), {}});
}

void Ui::bindKey(sf::Keyboard::Key key, Control const& control, std::string description)
{
    bindKey(key, description.empty() ? control.label() : std::move(description), control.id());
}

void Ui::bindKey(sf::Keyboard::Key key, std::string description, std::string controlId)
{
    addBinding({key, std::move(description), {}, std::move(controlId)});
}

void Ui::unbindKey(sf::Keyboard::Key key)
{
    std::lock_guard lock{m_mutex};
    m_keys.erase(std::remove_if(m_keys.begin(), m_keys.end(), [&](KeyBinding const& b) { return b.key == key; }),
                 m_keys.end());
}

void Ui::frame(sf::RenderTarget& target, double now, float dt, sf::Vector2f mouse)
{
    std::lock_guard    lock{m_mutex};
    UiContext const    ctx    = context(now, dt, mouse);
    float const        margin = m_theme.px(m_theme.margin);
    sf::Vector2f const size{target.getSize()};
    m_root->layout({{margin, margin}, {std::max(0.0f, size.x - 2 * margin), std::max(0.0f, size.y - 2 * margin)}},
                   ctx);
    m_root->update(ctx);
    m_root->draw(target, ctx);
}

} // namespace sml
