#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <SFML/System/Vector2.hpp>

namespace sml
{

class MetricStore;

/// A network as layers of nodes joined by edges (an MLP, a NEAT network, ...). Node order inside a
/// layer is the order of appearance. Values drive color (sign) and size (magnitude): node values
/// are activations, edge values are weights or the signal they carry.
struct LayeredGraph
{
    struct Node
    {
        int         layer = 0;
        float       value = 0.0f;
        std::string label; // drawn beside first- and last-layer nodes
    };
    struct Edge
    {
        int   from  = 0; // node indices
        int   to    = 0;
        float value = 0.0f;
    };

    std::vector<Node> nodes;
    std::vector<Edge> edges;

    /// Same nodes per layer, labels and edge endpoints (values may differ).
    [[nodiscard]] bool sameTopology(LayeredGraph const& o) const;
    [[nodiscard]] int  layerCount() const;
};

/// A chain of rigid links hanging from a base that slides along a rail (cart-pendulum), in world
/// units with y down.
struct LinkChainState
{
    sf::Vector2f              base;   // pivot on the cart
    std::vector<sf::Vector2f> joints; // end of each link, base outward; the last one is the tip
    float                     push = 0.0f; // horizontal disturbance at the tip (signed, 0 = none)
};

/// Progress of an iterative trainer (generations, epochs).
struct TrainingStats
{
    std::uint64_t iteration = 0;
    double        bestScore = 0.0;
    double        simTime   = 0.0; // seconds of simulated experience
    double        wallTime  = 0.0; // seconds of real time
    std::vector<std::pair<std::string, double>> extra;
};

/// Pushes `stats` as series `<prefix>iteration`, `best_score`, `sim_time`, `wall_time` and one per
/// extra, all at step = iteration.
void pushStats(MetricStore& store, TrainingStats const& stats, std::string_view prefix = {});

} // namespace sml
