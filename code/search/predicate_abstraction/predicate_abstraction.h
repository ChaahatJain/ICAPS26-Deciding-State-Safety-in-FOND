//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PREDICATE_ABSTRACTION_H
#define PLAJA_PREDICATE_ABSTRACTION_H

#include "base/search_pa_base.h"

class PredicateAbstraction: public SearchPABase {
    friend class PredicateAbstractionTest; //
    friend class PPASimpleTest; //
    friend class PPARacetrackTest; //
    friend class RelaxedPPARacetrackTest; //

private:
    SearchStatus finalize() override;

public:
    explicit PredicateAbstraction(const SearchEngineConfigPA& config);
    ~PredicateAbstraction() override;
    DELETE_CONSTRUCTOR(PredicateAbstraction)

    HeuristicValue_type get_goal_distance(const AbstractState& pa_state) override;

    /* Stats. */
    void print_statistics() const override;
    void stats_to_csv(std::ofstream& file) const override;
    void stat_names_to_csv(std::ofstream& file) const override;
};

#endif //PLAJA_PREDICATE_ABSTRACTION_H

