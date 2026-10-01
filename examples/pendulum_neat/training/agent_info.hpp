#pragma once
#include "common/number_generator.hpp"

#include "training/config.hpp"
#include "neat/genome.hpp"
#include "neat/network_generator.hpp"

#include "physic/configuration.hpp"


struct AgentInfo
{
    /// Attributes
    pbd::RealType score = 0.0f;
    nt::Genome    genome;

    /// Methods
    AgentInfo()
    {
        resetGenome();
    }

    [[nodiscard]]
    nt::Network generateNetwork()
    {
        return nt::NetworkGenerator().generate(genome);
    }

    void resetGenome()
    {
        genome = nt::Genome{conf::net::input_count, conf::net::output_count};
    }

    void createRandomFullConnections()
    {
        for (uint32_t i{0}; i < genome.info.inputs; ++i) {
            for (uint32_t k{0}; k < genome.info.outputs; ++k) {
                genome.createConnection(i, genome.info.inputs + k, RNGf::getFullRange(conf::mut::weight_range));
            }
        }
    }
};