//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SUCCESSOR_DISTRIBUTION_H
#define PLAJA_SUCCESSOR_DISTRIBUTION_H

#include "../../fd_adaptions/state.h"

struct SuccessorDistribution final {

private:
    utils::HashMap<std::unique_ptr<State>, PLAJA::Prob_type> probDistro;

public:
    SuccessorDistribution() = default;
    ~SuccessorDistribution() = default;

    inline void add_successor(std::unique_ptr<State>&& successor, PLAJA::Prob_type prob) {
        auto it = probDistro.find(successor);
        if (it == probDistro.end()) { probDistro.emplace(std::move(successor), prob); }
        else { it->second += prob; }
    }

    inline void scale_probability(PLAJA::Prob_type factor) { for (auto& state_prob: probDistro) { state_prob.second += factor; } }

    inline void merge(std::unique_ptr<SuccessorDistribution>&& other, PLAJA::Prob_type factor = 1.0) { for (auto& [state, prob]: other->probDistro) { add_successor(state->to_ptr(), prob * factor); } }

    [[nodiscard]] const utils::HashMap<std::unique_ptr<State>, PLAJA::Prob_type>& _probDistro() const { return probDistro; }

};


#endif //PLAJA_SUCCESSOR_DISTRIBUTION_H
