#pragma once
#include "simplyml/core/control_store.hpp"
#include "simplyml/core/event_bus.hpp"
#include "simplyml/core/metric_store.hpp"
#include "simplyml/core/resources.hpp"
#include "simplyml/ui/widget.hpp"

// A UiContext without a window: default theme, embedded font, empty stores.
struct TestUi
{
    sml::Theme        theme;
    sml::Resources    resources;
    sml::MetricStore  store;
    sml::ControlStore controls;
    sml::EventBus     events;
    double            now = 0.0;

    [[nodiscard]] sml::UiContext ctx()
    {
        return sml::UiContext{theme, resources.defaultFont(), store, now, 0.016f, {}, &controls, &events, nullptr};
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
