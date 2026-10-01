#pragma once
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "simplyml/ui/layout.hpp"
#include "simplyml/ui/panel.hpp"
#include "simplyml/ui/value_format.hpp"
#include "simplyml/util/smooth_value.hpp"

namespace sml
{

/// Interactive widget editing one named number. Its name is its id and its key in the Control
/// Store: user edits are published there (and posted as a custom App Event named after it, with
/// the new value), and writes to the store from elsewhere (training code, Python, another control
/// with the same name) show up in the widget on the next frame.
class Control : public Widget
{
public:
    Control(std::string name, std::string label);

    Control& setLabel(std::string label) { m_label = std::move(label); return *this; }
    [[nodiscard]] std::string const& label() const { return m_label; }

    [[nodiscard]] double value() const { return m_value; }
    /// Sets the value from code (no event, no callback); published to the store on the next update.
    Control& setValue(double value);

    /// Disabled controls ignore input and draw faded.
    Control& setEnabled(bool enabled) { m_enabled = enabled; return *this; }
    [[nodiscard]] bool enabled() const { return m_enabled; }

    /// Called on the UI thread after each user edit, with the new value.
    Control& onChange(std::function<void(double)> fn) { m_onChange = std::move(fn); return *this; }

    /// What a bound key does: press a button, flip a toggle, pick the next option. Default: nothing.
    virtual void trigger(UiContext const&) {}

    /// Syncs with the Control Store. Subclasses overriding it call this first.
    void update(UiContext const& ctx) override;

protected:
    /// A user edit: stores the (sanitized) value, publishes it, posts the event and calls `onChange`.
    /// No-op when the value doesn't change.
    void commit(double value, UiContext const& ctx);
    /// Initial value from a constructor: unlike `setValue`, a value already in the store wins.
    void initValue(double value) { m_value = sanitize(value); }
    /// Clamps/rounds a value into what the control can hold.
    [[nodiscard]] virtual double sanitize(double value) const { return value; }
    [[nodiscard]] bool interactive() const override { return m_enabled; }
    /// `c` with its alpha reduced when disabled.
    [[nodiscard]] sf::Color fade(sf::Color c) const;

private:
    std::string                 m_label;
    double                      m_value   = 0.0;
    std::uint64_t               m_seen    = 0;     // store version last read or written
    bool                        m_publish = false; // set from code; push on next update
    bool                        m_enabled = true;
    std::function<void(double)> m_onChange;
};

/// Push button. Its value counts clicks.
class Button : public Control
{
public:
    explicit Button(std::string name, std::string label = {}, sf::Color color = sf::Color::Transparent);

    Button& setColor(sf::Color color) { m_color = color; return *this; }
    [[nodiscard]] int clicks() const { return static_cast<int>(value()); }

    void trigger(UiContext const& ctx) override;
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

protected:
    void onClick(UiContext const& ctx) override;

private:
    sf::Color          m_color;
    SmoothValue<float> m_flash{0.0f, 0.35f, Ease::OutCubic}; // key-triggered press
};

/// Label with a pill switch on the right. Value 0 (off) or 1 (on).
class Toggle : public Control
{
public:
    explicit Toggle(std::string name, std::string label = {}, bool on = false);

    Toggle& setColors(sf::Color on, sf::Color off) { m_on = on; m_off = off; return *this; }
    [[nodiscard]] bool isOn() const { return value() > 0.5; }

    void trigger(UiContext const& ctx) override;
    void update(UiContext const& ctx) override;
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

protected:
    void onClick(UiContext const& ctx) override;
    [[nodiscard]] double sanitize(double v) const override { return v > 0.5 ? 1.0 : 0.0; }

private:
    sf::Color          m_on  = sf::Color::Transparent; // transparent = theme green / accent
    sf::Color          m_off = sf::Color::Transparent;
    SmoothValue<float> m_knob{0.0f, 0.18f, Ease::OutCubic};
};

/// Label and readout over a draggable track. Linear or log scale (log needs lo > 0), optional step.
/// Arrow keys nudge it while focused, the wheel while hovered.
class Slider : public Control
{
public:
    Slider(std::string name, std::string label, double lo, double hi, double value);

    Slider& setRange(double lo, double hi);
    Slider& setLog(bool log)               { m_log = log; return *this; }
    /// Values snap to multiples of `step` from `lo` (0 = continuous; 1 = integers).
    Slider& setStep(double step)           { m_step = step; return *this; }
    Slider& setFormat(ValueFormat fmt)     { m_format = std::move(fmt); return *this; }
    Slider& setColor(sf::Color color)      { m_color = color; return *this; }

    [[nodiscard]] double lo() const { return m_lo; }
    [[nodiscard]] double hi() const { return m_hi; }
    [[nodiscard]] double step() const { return m_step; }
    [[nodiscard]] bool   isLog() const { return m_log && m_lo > 0.0; }
    /// Track position in [0, 1] of `v`, and its inverse.
    [[nodiscard]] double ratioOf(double v) const;
    [[nodiscard]] double valueAt(double ratio) const;
    /// Readout text: the set format, else one fitting the range and scale.
    [[nodiscard]] std::string text(double v) const;

    bool handle(sf::Event const& event, UiContext const& ctx) override;
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

protected:
    [[nodiscard]] double sanitize(double v) const override;

private:
    [[nodiscard]] sf::FloatRect track(UiContext const& ctx) const; // knob centre's travel
    void dragTo(float x, UiContext const& ctx);
    void nudge(int steps, UiContext const& ctx);

    double                     m_lo, m_hi;
    bool                       m_log  = false;
    double                     m_step = 0.0;
    std::optional<ValueFormat> m_format;
    sf::Color                  m_color = sf::Color::Transparent; // transparent = theme blue
};

/// One of several options; the value is the option's index. Segmented: a row of buttons with a
/// sliding highlight. Radio: one option per line with a dot.
class Select : public Control
{
public:
    enum class Style
    {
        Segmented,
        Radio,
    };

    Select(std::string name, std::string label, std::vector<std::string> options, int index = 0,
           Style style = Style::Segmented);

    Select& setStyle(Style style)      { m_style = style; invalidate(); return *this; }
    Select& setColor(sf::Color color)  { m_color = color; return *this; }

    [[nodiscard]] std::vector<std::string> const& options() const { return m_options; }
    [[nodiscard]] int                             index() const   { return static_cast<int>(value()); }
    [[nodiscard]] std::string const&              selected() const;

    void trigger(UiContext const& ctx) override;
    bool handle(sf::Event const& event, UiContext const& ctx) override;
    void update(UiContext const& ctx) override;
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

    /// Option under `p`, or -1.
    [[nodiscard]] int optionAt(sf::Vector2f p, UiContext const& ctx) const;

protected:
    void onClick(UiContext const& ctx) override;
    [[nodiscard]] double sanitize(double v) const override;

private:
    [[nodiscard]] float         labelHeight(UiContext const& ctx) const;
    [[nodiscard]] sf::FloatRect optionRect(int i, UiContext const& ctx) const;

    std::vector<std::string> m_options;
    Style                    m_style;
    sf::Color                m_color = sf::Color::Transparent; // transparent = theme accent
    sf::Vector2f             m_pressPos;
    SmoothValue<float>       m_highlight{0.0f, 0.2f, Ease::OutCubic}; // selected index, animated
};

/// Typed number entry: click (or focus) to edit, Enter or clicking away to apply, Escape to cancel.
/// Text that isn't a number in range flashes red and is discarded.
class NumberField : public Control
{
public:
    NumberField(std::string name, std::string label, double value,
                double lo = -std::numeric_limits<double>::infinity(),
                double hi = std::numeric_limits<double>::infinity());

    NumberField& setRange(double lo, double hi) { m_lo = lo; m_hi = hi; return *this; }
    NumberField& setInteger(bool integer)       { m_integer = integer; return *this; }
    NumberField& setFormat(ValueFormat fmt)     { m_format = std::move(fmt); return *this; }

    [[nodiscard]] bool               integer() const { return m_integer; }
    [[nodiscard]] bool               editing() const { return focused(); }
    [[nodiscard]] std::string const& editText() const { return m_edit; }
    /// Parses `text`; nullopt unless it is a whole finite number within range.
    [[nodiscard]] std::optional<double> parse(std::string const& text) const;

    bool handle(sf::Event const& event, UiContext const& ctx) override;
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

protected:
    void onFocusChanged(bool focused, UiContext const& ctx) override;
    [[nodiscard]] double sanitize(double v) const override;

private:
    void apply(UiContext const& ctx);

    double      m_lo, m_hi;
    bool        m_integer = false;
    ValueFormat m_format  = ValueFormat::general(6);
    std::string m_edit;
    bool        m_replace    = false; // first keystroke replaces the text
    bool        m_cancelled  = false;
    double      m_errorUntil = -1.0;
};

/// Panel stacking controls top to bottom, each at its natural height.
class ControlPanel : public Panel
{
public:
    explicit ControlPanel(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
        T& w = m_rows->add<T>(std::forward<Args>(args)...);
        w.setExtent(fit());
        return w;
    }
    Button&      addButton(std::string name, std::string label = {});
    Toggle&      addToggle(std::string name, std::string label, bool on = false);
    Slider&      addSlider(std::string name, std::string label, double lo, double hi, double value);
    Select&      addSelect(std::string name, std::string label, std::vector<std::string> options, int index = 0);
    NumberField& addNumber(std::string name, std::string label, double value);

    [[nodiscard]] Column& rows() { return *m_rows; }

    /// Height fitting every visible row.
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;

protected:
    void onLayout(UiContext const& ctx) override;

private:
    Column* m_rows;
};

} // namespace sml
