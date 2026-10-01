#include "simplyml/ml/training_stats_card.hpp"

namespace sml
{

TrainingStatsCard::TrainingStatsCard(std::string title, sf::Color accent, std::string const& prefix,
                                     std::string iterationLabel)
    : StatCard{std::move(title), accent}
    , m_iteration{&addBig(std::move(iterationLabel), prefix + "iteration")}
    , m_best{&addValue("Best score", prefix + "best_score", ValueFormat::number(4))}
    , m_sim{&addValue("Simulated training time", prefix + "sim_time", ValueFormat::duration())}
    , m_wall{&addValue("Real time training time", prefix + "wall_time", ValueFormat::duration())}
{}

} // namespace sml
