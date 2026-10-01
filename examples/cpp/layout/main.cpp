// A grid of panels that reflows as the window resizes: a fixed-height header row, then a grid
// whose column count follows the window width. Esc quits.
#include <simplyml/core/app.hpp>
#include <simplyml/ui/layout.hpp>
#include <simplyml/ui/panel.hpp>
#include <simplyml/ui/ui.hpp>

int main()
{
    sml::AppConfig config;
    config.title          = "SimplyML - layout";
    config.size           = {1400, 800};
    config.escToQuit      = true;
    config.cameraControls = false;
    sml::App app{config};
    sml::Ui  ui{app};
    auto const& pal = ui.theme().palette;

    auto& root = ui.setRoot<sml::Column>();

    auto& header = root.add<sml::Row>();
    header.setExtent(sml::px(110));
    header.add<sml::Panel>("Generation", pal.teal).setExtent(sml::px(300));
    header.add<sml::Panel>("Status", pal.green);
    header.add<sml::Panel>("Time", pal.yellow).setExtent(sml::px(300));

    auto& grid = root.add<sml::Grid>();
    grid.setAutoColumns(380, 4);
    grid.add<sml::Panel>("Score", pal.blue).setSpan(2).setId("score");
    static_cast<sml::Panel&>(ui["score"]).setValueText("0.98765");
    grid.add<sml::Panel>("Loss", pal.accent);
    grid.add<sml::Panel>("Accuracy", pal.teal);
    grid.add<sml::Panel>("Gravity", pal.yellow);
    grid.add<sml::Panel>("Friction", pal.orange);
    grid.add<sml::Panel>("Network", pal.green).setSpan(2);

    app.run();
}
