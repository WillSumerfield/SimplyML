#include <doctest/doctest.h>

#include <cmath>

#include "helpers.hpp"
#include "simplyml/core/app.hpp"
#include "simplyml/ui/controls.hpp"
#include "simplyml/ui/key_bindings.hpp"
#include "simplyml/ui/ui.hpp"

namespace
{

sf::Event press(sf::Vector2f p)
{
    return sf::Event::MouseButtonPressed{sf::Mouse::Button::Left, sf::Vector2i(p)};
}
sf::Event release(sf::Vector2f p)
{
    return sf::Event::MouseButtonReleased{sf::Mouse::Button::Left, sf::Vector2i(p)};
}
sf::Event move(sf::Vector2f p)
{
    return sf::Event::MouseMoved{sf::Vector2i(p)};
}
sf::Event key(sf::Keyboard::Key k)
{
    return sf::Event::KeyPressed{k, sf::Keyboard::Scancode::Unknown, false, false, false, false};
}
sf::Event text(char c)
{
    return sf::Event::TextEntered{static_cast<char32_t>(c)};
}

template<typename W>
void click(W& w, sml::UiContext const& ctx, sf::Vector2f p)
{
    w.handle(press(p), ctx);
    w.handle(release(p), ctx);
}

} // namespace

TEST_CASE("Slider: linear/log mapping, step, sanitize, readout")
{
    sml::Slider s{"x", "X", 0.0, 10.0, 5.0};
    CHECK(s.ratioOf(5.0) == doctest::Approx(0.5));
    CHECK(s.valueAt(0.25) == doctest::Approx(2.5));
    CHECK(s.ratioOf(-3.0) == 0.0);
    s.setValue(20.0);
    CHECK(s.value() == 10.0);
    s.setValue(std::nan(""));
    CHECK(s.value() == 10.0); // non-finite ignored
    s.setStep(2.0).setValue(3.1);
    CHECK(s.value() == 4.0);
    CHECK(s.text(4.0) == "4");

    sml::Slider lr{"lr", "LR", 1e-5, 1e-1, 1e-3};
    lr.setLog(true);
    CHECK(lr.ratioOf(1e-3) == doctest::Approx(0.5));
    CHECK(lr.valueAt(0.25) == doctest::Approx(1e-4));
    CHECK(lr.text(1e-4) == "0.0001");
    lr.setRange(0.0, 1.0);
    CHECK_FALSE(lr.isLog()); // log needs lo > 0
}

TEST_CASE("Control: publishes to and adopts from the Control Store; user edits post events")
{
    TestUi t;
    t.events.setQueueEnabled(true);
    sml::Toggle tog{"pause", "Pause"};
    tog.layout({{0, 0}, {300, 40}}, t.ctx());

    tog.update(t.ctx()); // first update publishes the initial value
    CHECK(t.controls.get("pause", -1) == 0.0);

    click(tog, t.ctx(), {150, 20});
    CHECK(tog.isOn());
    CHECK(t.controls.get("pause", -1) == 1.0);
    auto ev = t.events.drain();
    REQUIRE(ev.size() == 1);
    CHECK(ev[0].name == "pause");
    CHECK(ev[0].value == 1.0);

    t.controls.set("pause", 0.0); // e.g. from Python
    tog.update(t.ctx());
    CHECK_FALSE(tog.isOn());
    CHECK(t.events.drain().empty()); // external writes post nothing

    t.controls.set("pause", 7.0); // sanitized and re-published
    tog.update(t.ctx());
    CHECK(tog.isOn());
    CHECK(t.controls.get("pause", -1) == 1.0);

    sml::Slider s{"lr", "LR", 0.0, 1.0, 0.5};
    t.controls.set("lr", 0.25); // preset before the widget's first frame wins
    s.update(t.ctx());
    CHECK(s.value() == 0.25);

    int calls = 0;
    sml::Button b{"reset"};
    b.onChange([&](double) { ++calls; });
    b.layout({{0, 0}, {200, 44}}, t.ctx());
    click(b, t.ctx(), {100, 22});
    click(b, t.ctx(), {100, 22});
    CHECK(b.clicks() == 2);
    CHECK(calls == 2);
    CHECK(b.label() == "reset");

    b.setEnabled(false);
    click(b, t.ctx(), {100, 22});
    CHECK(b.clicks() == 2);
}

TEST_CASE("Slider: press, drag outside, wheel and keys")
{
    TestUi t;
    sml::Slider s{"x", "X", 0.0, 100.0, 0.0};
    s.layout({{0, 0}, {220, 60}}, t.ctx()); // knob travels x in [10, 210]
    s.handle(press({110, 40}), t.ctx());
    CHECK(s.value() == doctest::Approx(50.0));
    s.handle(move({500, 40}), t.ctx()); // still dragging outside the bounds
    CHECK(s.value() == 100.0);
    s.handle(release({500, 40}), t.ctx());
    s.handle(move({500, 40}), t.ctx());
    CHECK(s.value() == 100.0);

    CHECK(s.focused());
    s.handle(key(sf::Keyboard::Key::Left), t.ctx());
    CHECK(s.value() == doctest::Approx(99.0));
    s.handle(key(sf::Keyboard::Key::Home), t.ctx());
    CHECK(s.value() == 0.0);

    s.handle(move({50, 40}), t.ctx());
    s.handle(sf::Event::MouseWheelScrolled{sf::Mouse::Wheel::Vertical, 1.0f, {50, 40}}, t.ctx());
    CHECK(s.value() == doctest::Approx(1.0));
}

TEST_CASE("Select: click picks the option under the pointer; trigger cycles")
{
    TestUi t;
    sml::Select s{"opt", "Optimizer", {"sgd", "adam", "rmsprop"}};
    s.layout({{0, 0}, {300, 80}}, t.ctx());
    CHECK(s.index() == 0);
    int const i = s.optionAt({250, 45}, t.ctx());
    CHECK(i == 2);
    click(s, t.ctx(), {250, 45});
    CHECK(s.selected() == "rmsprop");
    s.trigger(t.ctx());
    CHECK(s.index() == 0);
    s.setValue(10);
    CHECK(s.index() == 2);

    s.setStyle(sml::Select::Style::Radio);
    s.layout({{0, 0}, {300, 200}}, t.ctx());
    CHECK(s.optionAt({20, s.naturalSize(t.ctx()).y - 5}, t.ctx()) == 2);
}

TEST_CASE("NumberField: type + Enter applies, bad text flashes, Escape cancels, typing swallows keys")
{
    TestUi t;
    sml::NumberField f{"bs", "Batch", 32, 1, 1024};
    f.setInteger(true);
    f.layout({{0, 0}, {200, 80}}, t.ctx());
    click(f, t.ctx(), {100, 60});
    REQUIRE(f.editing());
    CHECK(f.editText() == "32");
    CHECK(f.handle(key(sf::Keyboard::Key::Num6), t.ctx())); // SFML: KeyPressed, then TextEntered
    CHECK(f.handle(text('6'), t.ctx()));                     // replaces the selected text
    f.handle(text('4'), t.ctx());
    f.handle(text('x'), t.ctx()); // ignored
    CHECK(f.editText() == "64");
    CHECK(f.handle(key(sf::Keyboard::Key::S), t.ctx())); // a hotkey letter never escapes
    f.handle(key(sf::Keyboard::Key::Enter), t.ctx());
    CHECK_FALSE(f.editing());
    CHECK(f.value() == 64);
    CHECK(t.controls.get("bs", 0) == 64);

    click(f, t.ctx(), {100, 60});
    f.handle(text('9'), t.ctx());
    f.handle(text('9'), t.ctx());
    f.handle(text('9'), t.ctx());
    f.handle(text('9'), t.ctx()); // 9999 > 1024
    f.handle(key(sf::Keyboard::Key::Enter), t.ctx());
    CHECK(f.value() == 64);

    click(f, t.ctx(), {100, 60});
    f.handle(text('1'), t.ctx());
    f.handle(key(sf::Keyboard::Key::Escape), t.ctx());
    CHECK(f.value() == 64);

    click(f, t.ctx(), {100, 60});
    f.handle(key(sf::Keyboard::Key::Backspace), t.ctx());
    f.handle(text('8'), t.ctx());
    click(f, t.ctx(), {900, 900}); // clicking away applies
    CHECK(f.value() == 8);

    CHECK(f.parse("1e2") == 100.0);
    CHECK_FALSE(f.parse("1e"));
    CHECK_FALSE(f.parse(""));
}

TEST_CASE("Ui key bindings: actions, control triggers, legend-only pass-through, focus wins")
{
    sml::App app;
    sml::Ui  ui{app};
    auto&    panel = ui.setRoot<sml::Column>().add<sml::ControlPanel>("Controls");
    auto&    tog   = panel.addToggle("pause", "Pause");
    auto&    sel   = panel.addSelect("opt", "Opt", {"a", "b"});
    int      saves = 0;
    ui.bindKey(sf::Keyboard::Key::Space, tog);
    ui.bindKey(sf::Keyboard::Key::O, sel, "Cycle optimizer");
    ui.bindKey(sf::Keyboard::Key::S, "Save", [&] { ++saves; });
    ui.bindKey(sf::Keyboard::Key::D, "Dump (handled by the app)");

    CHECK(app.events().dispatch(key(sf::Keyboard::Key::Space)));
    CHECK(tog.isOn());
    CHECK(app.controls().get("pause", -1) == 1.0);
    CHECK(app.events().dispatch(key(sf::Keyboard::Key::O)));
    CHECK(sel.index() == 1);
    CHECK(app.events().dispatch(key(sf::Keyboard::Key::S)));
    CHECK(saves == 1);
    CHECK_FALSE(app.events().dispatch(key(sf::Keyboard::Key::D)));

    REQUIRE(ui.keyBindings().size() == 4);
    CHECK(ui.keyBindings()[0].description == "Pause");
    CHECK(ui.keyBindings()[1].description == "Cycle optimizer");
    ui.bindKey(sf::Keyboard::Key::S, "Save v2", [&] { saves += 10; });
    CHECK(ui.keyBindings().size() == 4);
    app.events().dispatch(key(sf::Keyboard::Key::S));
    CHECK(saves == 11);
    ui.unbindKey(sf::Keyboard::Key::S);
    CHECK_FALSE(app.events().dispatch(key(sf::Keyboard::Key::S)));

    CHECK(sml::keyLabel(sf::Keyboard::Key::Space) == "Space");
    CHECK(sml::keyLabel(sf::Keyboard::Key::S) == "S");
    CHECK(sml::keyLabel(sf::Keyboard::Key::Num4) == "4");
}

TEST_CASE("ControlPanel natural height grows with rows")
{
    TestUi t;
    sml::ControlPanel p{"Controls"};
    float const empty = p.naturalSize(t.ctx()).y;
    p.addSlider("a", "A", 0, 1, 0);
    float const one = p.naturalSize(t.ctx()).y;
    p.addButton("b");
    CHECK(one > empty);
    CHECK(p.naturalSize(t.ctx()).y > one);
}
