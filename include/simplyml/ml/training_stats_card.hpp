#pragma once
#include <string>

#include "simplyml/ui/stats.hpp"

namespace sml
{

/// StatCard preset for TrainingStats pushed with `pushStats`: the iteration as a headline, the best
/// score, and simulated and real training time. Further rows (gauges, extras) append below; move
/// them with `rows().move(...)`.
class TrainingStatsCard : public StatCard
{
public:
    explicit TrainingStatsCard(std::string title = "Iteration", sf::Color accent = sf::Color::Transparent,
                               std::string const& prefix = {}, std::string iterationLabel = {});

    [[nodiscard]] StatValue& iteration() { return *m_iteration; }
    [[nodiscard]] StatValue& bestScore() { return *m_best; }
    [[nodiscard]] StatValue& simTime()   { return *m_sim; }
    [[nodiscard]] StatValue& wallTime()  { return *m_wall; }

private:
    StatValue* m_iteration;
    StatValue* m_best;
    StatValue* m_sim;
    StatValue* m_wall;
};

} // namespace sml
