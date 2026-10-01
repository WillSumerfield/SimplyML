#include "simplyml/ui/ui.hpp"

#include <algorithm>

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
    m_eventId = app.events().subscribe([this](sf::Event const& e) {
        std::lock_guard lock{m_mutex};
        return m_root->handle(e, context(m_app.clock().now(), m_app.clock().dt(), m_app.mouseScreen()));
    });
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

UiContext Ui::context(double now, float dt, sf::Vector2f mouse) const
{
    return UiContext{m_theme, *m_theme.font, m_app.store(), now, dt, mouse};
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
