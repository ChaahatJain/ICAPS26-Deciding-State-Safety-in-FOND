//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_MODEL_Z3_PA_H
#define PLAJA_MODEL_Z3_PA_H

#include <unordered_set>
#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "../../information/forward_information.h"
#include "../../smt/model/model_z3.h"
#include "../../using_search.h"
#include "../successor_generation/forward_succ_gen_pa.h"
#include "../pa_states/forward_pa_states.h"
#include "../using_predicate_abstraction.h"
#include "forward_smt_pa.h"

class ModelZ3PA: public ModelZ3 {

private:
    // state info for underlying abstraction model: locations and predicates
    std::unique_ptr<PaStateValues> paInitialState; // of locations and predicates for "first" initial state (*warning* does not fulfill closure under lazy pa)
    std::vector<PLAJA::integer> lowerBounds; // the minimal elements in the domain of the variables; *0* for Booleans
    std::vector<PLAJA::integer> upperBounds; // the maximal elements in the domain of the variables; *1* for Booleans
    std::vector<Range_type> ranges; // ranges, i.e, the domain size for bounded integers and booleans, note: *int* is legacy due to IntPacker from FD

    // predicates ...
    std::vector<std::vector<std::unique_ptr<z3::expr>>> predicatesInZ3PerStep;
    // Auxiliary expression deprecated:
    // index bounds must be part of predicate.
    // example: predicates A[x]==2 with array length 2,x==3 and assignment x=3,
    // given concrete state (x=3,_) we can self-loop.
    // However, if auxiliary predicate 0 <= x < 2 is not part of negation, then there is no self loop for abstract state (x==3,!(A[x]==2)).
    // z3::expr_vector targetPredAux;
    // z3::expr_vector srcPredAux;

    const std::vector<std::unique_ptr<z3::expr>>& get_predicates_z3(StepIndex_type step) const {
        PLAJA_ASSERT(step < predicatesInZ3PerStep.size())
        return predicatesInZ3PerStep[step];
    }

    const std::vector<std::unique_ptr<z3::expr>>& _src_predicates() const { return get_predicates_z3(0); }

    void step_predicates(bool reset);
    void compute_abstract_model_info(bool reset); // to be called after step_predicates

public:
    explicit ModelZ3PA(const PLAJA::Configuration& config, const PredicatesExpression* predicates_ = nullptr);
    ~ModelZ3PA() override;
    DELETE_CONSTRUCTOR(ModelZ3PA)

    void generate_steps(StepIndex_type max_step) override;

    void increment();

    // getter abstract model info
    [[nodiscard]] inline const PaStateValues* get_pa_initial_state() const { return paInitialState.get(); }

    [[nodiscard]] std::unique_ptr<PaStateValues> compute_abstract_state_values(const StateValues& concrete_state) const;

    [[nodiscard]] inline const std::vector<PLAJA::integer>& get_lower_bounds() const { return lowerBounds; }

    [[nodiscard]] inline const std::vector<PLAJA::integer>& get_upper_bounds() const { return upperBounds; }

    [[nodiscard]] inline const std::vector<Range_type>& get_ranges() const { return ranges; }

    // predicates ...
    [[nodiscard]] inline const z3::expr& get_predicate_z3(PredicateIndex_type pred_index, StepIndex_type step) const {
        PLAJA_ASSERT(pred_index < get_predicates_z3(step).size())
        return *get_predicates_z3(step)[pred_index];
    }

    [[nodiscard]] inline const z3::expr& get_src_predicate(PredicateIndex_type index) const { return get_predicate_z3(index, 0); }

    [[nodiscard]] inline const z3::expr& get_target_predicate(PredicateIndex_type index) const { return get_predicate_z3(index, 1); }

    [[nodiscard]] const ActionOpPA& get_action_op_pa(ActionOpID_type op_id) const;

    void add_to(const AbstractState& abstract_state, Z3_IN_PLAJA::SMTSolver& solver, StepIndex_type step = 0) const;

};

namespace MODEL_Z3_PA { extern void make_sharable(const PLAJA::Configuration& config); } // to hide internals of ModelZ3PA in main CEGAR


#endif //PLAJA_MODEL_Z3_PA_H
