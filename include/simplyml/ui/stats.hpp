#pragma once
#include <optional>
#include <string>

#include "simplyml/ui/layout.hpp"
#include "simplyml/ui/panel.hpp"
#include "simplyml/ui/value_format.hpp"
#include "simplyml/util/smooth_value.hpp"

namespace sml
{

/// Widget showing one value: the latest point of a Metric Store series, a value set directly, or
/// fixed text (first set wins, in that order: text, series, value).
class ValueWidget : public Widget
{
public:
    ValueWidget& setLabel(std::string label)   { m_label = std::move(label); return *this; }
    ValueWidget& setSeries(std::string series) { m_series = std::move(series); return *this; }
    ValueWidget& setValue(double value)        { m_value = value; return *this; }
    ValueWidget& setText(std::string text)     { m_text = std::move(text); return *this; }
    ValueWidget& setFormat(ValueFormat fmt)    { m_format = std::move(fmt); return *this; }

    [[nodiscard]] std::string const& label() const  { return m_label; }
    [[nodiscard]] std::string const& series() const { return m_series; }

    /// Current number (series' latest, else the set value), if any.
    [[nodiscard]] std::optional<double> current(UiContext const& ctx) const;
    /// Text to show: fixed text, else the formatted number, else "-".
    [[nodiscard]] std::string currentText(UiContext const& ctx) const;

protected:
    std::string           m_label;
    std::string           m_series;
    std::optional<double> m_value;
    std::optional<std::string> m_text;
    ValueFormat           m_format;
};

/// Label and value. `Stacked`: small dim label over a value. `Big`: optional label over a large
/// headline value. `Inline`: label on the left, large value on the right (stat tiles).
class StatValue : public ValueWidget
{
public:
    enum class Style
    {
        Stacked,
        Big,
        Inline,
    };

    explicit StatValue(std::string label = {}, std::string series = {}, ValueFormat fmt = {},
                       Style style = Style::Stacked);

    StatValue& setStyle(Style style) { m_style = style; return *this; }

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    Style m_style;
};

/// Labelled progress bar for a value in [lo, hi] (clamped), animating as it changes.
class Gauge : public ValueWidget
{
public:
    explicit Gauge(std::string label = {}, std::string series = {}, double lo = 0.0, double hi = 1.0,
                   sf::Color color = sf::Color::Transparent);

    Gauge& setRange(double lo, double hi) { m_lo = lo; m_hi = hi; return *this; }
    Gauge& setColor(sf::Color color)      { m_color = color; return *this; }
    /// Shows the formatted value right of the label (default on).
    Gauge& setShowValue(bool show)        { m_showValue = show; return *this; }

    /// Fill ratio in [0, 1] for `v`.
    [[nodiscard]] double ratio(double v) const;

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void update(UiContext const& ctx) override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    double             m_lo, m_hi;
    sf::Color          m_color;
    bool               m_showValue = true;
    SmoothValue<float> m_ratio{0.0f, 0.3f, Ease::OutCubic};
};

/// On/off indicator: label, colored dot and state text. On when the value is above `threshold`.
class StatusDot : public ValueWidget
{
public:
    explicit StatusDot(std::string label = {}, std::string series = {}, std::string onText = "Enabled",
                       std::string offText = "Disabled");

    StatusDot& setThreshold(double threshold) { m_threshold = threshold; return *this; }
    StatusDot& setColors(sf::Color on, sf::Color off) { m_on = on; m_off = off; return *this; }
    StatusDot& setTexts(std::string on, std::string off) { m_onText = std::move(on); m_offText = std::move(off); return *this; }

    [[nodiscard]] bool isOn(UiContext const& ctx) const;

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;
    void draw(sf::RenderTarget& target, UiContext const& ctx) override;

private:
    std::string m_onText, m_offText;
    double      m_threshold = 0.5;
    sf::Color   m_on  = sf::Color::Transparent; // transparent = theme green / accent
    sf::Color   m_off = sf::Color::Transparent;
};

/// Panel stacking stat rows top to bottom: values, headline values, gauges and status dots.
class StatCard : public Panel
{
public:
    explicit StatCard(std::string title = {}, sf::Color accent = sf::Color::Transparent);

    StatValue& addValue(std::string label, std::string series, ValueFormat fmt = {});
    StatValue& addBig(std::string label, std::string series, ValueFormat fmt = ValueFormat::integer());
    Gauge&     addGauge(std::string label, std::string series, double lo, double hi,
                        sf::Color color = sf::Color::Transparent);
    StatusDot& addStatus(std::string label, std::string series, std::string onText = "Enabled",
                         std::string offText = "Disabled");
    /// Any other widget as a row, at its natural height.
    template<typename T, typename... Args>
    T& addRow(Args&&... args)
    {
        T& w = m_rows->add<T>(std::forward<Args>(args)...);
        w.setExtent(fit());
        return w;
    }

    [[nodiscard]] Column& rows() { return *m_rows; }

    /// Height of the visible rows plus chrome, for `fit()`.
    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;

protected:
    void onLayout(UiContext const& ctx) override;

private:
    Column* m_rows;
};

/// Compact panel with one label and one large value side by side (e.g. "Generation 0042").
class StatTile : public Panel
{
public:
    explicit StatTile(std::string label = {}, std::string series = {}, ValueFormat fmt = ValueFormat::integer(),
                      sf::Color accent = sf::Color::Transparent);

    [[nodiscard]] StatValue& value() { return *m_value; }

    [[nodiscard]] sf::Vector2f naturalSize(UiContext const& ctx) const override;

private:
    StatValue* m_value;
};

} // namespace sml
