#include "simplyml/ml/formats.hpp"

#include <algorithm>

#include "simplyml/core/metric_store.hpp"

namespace sml
{

// LayeredGraph ----------------------------------------------------------------------------------------

bool LayeredGraph::sameTopology(LayeredGraph const& o) const
{
    if (nodes.size() != o.nodes.size() || edges.size() != o.edges.size()) {
        return false;
    }
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].layer != o.nodes[i].layer || nodes[i].label != o.nodes[i].label) {
            return false;
        }
    }
    for (std::size_t i = 0; i < edges.size(); ++i) {
        if (edges[i].from != o.edges[i].from || edges[i].to != o.edges[i].to) {
            return false;
        }
    }
    return true;
}

int LayeredGraph::layerCount() const
{
    std::vector<int> layers;
    for (auto const& n : nodes) {
        layers.push_back(n.layer);
    }
    std::sort(layers.begin(), layers.end());
    return static_cast<int>(std::unique(layers.begin(), layers.end()) - layers.begin());
}

void pushStats(MetricStore& store, TrainingStats const& stats, std::string_view prefix)
{
    auto const step = static_cast<double>(stats.iteration);
    auto const name = [&](std::string_view n) { return std::string(prefix) + std::string(n); };
    store.push(name("iteration"), step, step);
    store.push(name("best_score"), step, stats.bestScore);
    store.push(name("sim_time"), step, stats.simTime);
    store.push(name("wall_time"), step, stats.wallTime);
    for (auto const& [key, value] : stats.extra) {
        store.push(name(key), step, value);
    }
}

} // namespace sml
