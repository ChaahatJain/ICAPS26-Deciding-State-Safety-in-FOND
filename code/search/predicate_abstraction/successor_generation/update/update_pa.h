//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_UPDATE_PA_H
#define PLAJA_UPDATE_PA_H

#include <list>
#include <unordered_map>
#include "../../optimization/forward_optimization_pa.h"
#include "../../pa_states/forward_pa_states.h"
#include "../../smt/forward_smt_pa.h"
#include "../forward_succ_gen_pa.h"
#include "update_abstract.h"

namespace UPDATE_PA {
    struct EntailmentInfo;
    struct UpdateInZ3;
}

class UpdatePA final: public UpdateAbstract {
    friend ActionOpPA; //
    friend UPDATE_PA::UpdateInZ3;

private:
    // Z3 cache:
    std::unique_ptr<UPDATE_PA::UpdateInZ3> updateInZ3PA;

    // optimization
    static bool predicateEntailmentsFlag; // optimization flags
    unsigned int lastAffectingPred; // the predicate with the largest index variable-intersecting the update (including operator guard)
    std::list<UPDATE_PA::EntailmentInfo> staticPredEntailments; // "static": cached at finalization time
    std::list<PredicateIndex_type> invariantPreds; // frequent special case of entailment (in particular if variable disjoint)

    // finalization routines:
    void finalize(Z3_IN_PLAJA::SMTSolver& solver); // ass: solver asserts guard (and source variable bounds) of the underlying action operator

public:
    UpdatePA(const Update& parent, const ModelZ3PA& model_z3, Z3_IN_PLAJA::SMTSolver& solver);
    ~UpdatePA() override;
    DELETE_CONSTRUCTOR(UpdatePA)

    static void set_flags();

    void increment(Z3_IN_PLAJA::SMTSolver& solver);

    [[nodiscard]] const ModelZ3PA& get_model_z3_pa() const;
    // some short-cuts to model pa:
    [[nodiscard]] std::size_t get_number_predicates() const;
    [[nodiscard]] const Expression* get_predicate(PredicateIndex_type index) const;

    [[nodiscard]] bool is_always_entailed(PredicateIndex_type pred_index) const;

    /**
     * Add the respective target predicate to solver.
     */
    void add_predicate_to_solver(Z3_IN_PLAJA::SMTSolver& solver, PredicateIndex_type pred_index, bool value) const;

    /**
     * Add target predicates to solver.
     */
    void add_predicates_to_solver(Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& target) const;

    bool check_sat(Z3_IN_PLAJA::SMTSolver& solver, PredicateIndex_type pred_index, bool value) const;

    bool check_sat(Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& target) const;

    /**
     * Fix predicate truth values according to source state + update aware entailment.
     * Ignores values already defined (though asserted to be entailed).
     * @param solver must contain source state and operator constraints.
     */
    void fix(PredicateState& target, Z3_IN_PLAJA::SMTSolver& solver, const AbstractState& source, const PredEntailmentCacheInterface& entailment_cache_interface) const;

};

#endif //PLAJA_UPDATE_PA_H
