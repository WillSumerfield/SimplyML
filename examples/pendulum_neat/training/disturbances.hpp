#pragma once
#include <vector>
#include "common/number_generator.hpp"

#include "physic/configuration.hpp"


struct Disturbances
{
    struct Push
    {
        pbd::RealType delay    = 0.0f;
        pbd::RealType force    = 0.0f;
        pbd::RealType duration = 0.0f;
    };

    std::vector<Push> pushes;

    Disturbances() = default;

    void generateSequence()
    {
        size_t const count = 1000;
        pushes.resize(count);
        pushes[0].delay = 0.0f;
        for (size_t i{1}; i < count; ++i) {
            pushes[i].delay  = RNGf::getRange(2.0f, 5.0f);
            pushes[i].force = RNGf::getRange(15.0f, 40.0f);
            pushes[i].duration = RNGf::getRange(0.2f, 0.5f);
            if (RNGf::proba(0.5f)) {
                pushes[i].force *= -1.0f;
            }
        }
    }
};
