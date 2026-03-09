//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_HAMMING_DISTANCE_H
#define PLAJA_HAMMING_DISTANCE_H

#include <list>
#include "../../../utils/default_constructors.h"
#include "../optimization/forward_optimization_pa.h"
#include "../smt/forward_smt_pa.h"
#include "../using_predicate_abstraction.h"
#include "abstract_heuristic.h"

class HammingDistance final: public AbstractHeuristic {
private:
    std::unique_ptr<PaEntailments> entailments;
    std::shared_ptr<PredicateRelations> predicateRelations; // Only for lazy pa.

public:
    explicit HammingDistance(const PLAJA::Configuration& config, const ModelZ3PA& model_pa);
    ~HammingDistance() final;
    DELETE_CONSTRUCTOR(HammingDistance)

    void increment() override;

    [[nodiscard]] HeuristicValue_type _evaluate(const AbstractState& state) final;
    [[nodiscard]] HeuristicValue_type _evaluate(const AbstractState& state) const final;

    [[nodiscard]] static std::unique_ptr<HeuristicStateQueue> make_heuristic_state_queue(PLAJA::Configuration& config, const ModelZ3PA& model_pa);

};

#endif //PLAJA_HAMMING_DISTANCE_H
