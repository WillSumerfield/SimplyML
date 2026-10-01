#pragma once
#include "simplyml/core/metric_store.hpp"
#include "simplyml/core/resources.hpp"
#include "simplyml/ui/widget.hpp"

// A UiContext without a window: default theme, embedded font, empty store.
struct TestUi
{
    sml::Theme       theme;
    sml::Resources   resources;
    sml::MetricStore store;
    double           now = 0.0;

    [[nodiscard]] sml::UiContext ctx() const
    {
        return sml::UiContext{theme, resources.defaultFont(), store, now, 0.016f, {}};
    }
};

// Leaf that records layouts and clicks.
class Probe : public sml::Widget
{
public:
    int  layouts = 0;
    int  clicks  = 0;
    bool clickable = false;

    void draw(sf::RenderTarget&, sml::UiContext const&) override {}

protected:
    void onLayout(sml::UiContext const&) override { ++layouts; }
    bool interactive() const override { return clickable; }
    void onClick(sml::UiContext const&) override { ++clicks; }
};
