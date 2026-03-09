//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#ifndef PLAJA_MODEL_VERITAS_PA_H
#define PLAJA_MODEL_VERITAS_PA_H

#include <memory>
#include <vector>
#include "../../../../include/pa_config_const.h"
#include "../../../../parser/ast/expression/forward_expression.h"
#include "../../../smt_ensemble/model/model_veritas.h"
#include "../../../smt_ensemble/forward_smt_veritas.h"
#include "../../../information/jani2ensemble/using_jani2ensemble.h"
#include "../../../successor_generation/forward_successor_generation.h"
#include "../../using_predicate_abstraction.h"
#include "../../pa_states/forward_pa_states.h"
#include "forward_smt_ensemble_pa.h"

class ModelVeritasPA final: public ModelVeritas {

private:
    std::vector<std::vector<std::unique_ptr<VERITAS_IN_PLAJA::PredicateConstraint>>> predicatesInVeritasPerStep;

    [[nodiscard]] const std::vector<std::unique_ptr<VERITAS_IN_PLAJA::PredicateConstraint>>& get_predicates_in_veritas(StepIndex_type step) const {
        PLAJA_ASSERT(step < predicatesInVeritasPerStep.size())
        return predicatesInVeritasPerStep[step];
    }

    void step_predicates(bool reset);

public:
    explicit ModelVeritasPA(const PLAJA::Configuration& config, const PredicatesExpression* predicates_ = nullptr);
    ~ModelVeritasPA() override;
    DELETE_CONSTRUCTOR(ModelVeritasPA)

    void generate_steps(StepIndex_type max_step) override;

    void increment();

    [[nodiscard]] inline std::size_t get_num_predicates_veritas() const { return get_predicates_in_veritas(0).size(); }

    [[nodiscard]] inline const VERITAS_IN_PLAJA::PredicateConstraint* get_predicate_in_veritas(PredicateIndex_type pred_index, StepIndex_type step = 0) const {
        PLAJA_ASSERT(pred_index < get_predicates_in_veritas(step).size())
        return get_predicates_in_veritas(step)[pred_index].get();
    }

    [[nodiscard]] const ActionOpVeritasPA& get_action_op_pa(ActionOpID_type op_id) const;

    void add_to(const PaStateBase& pa_state, VERITAS_IN_PLAJA::QueryConstructable& query, StepIndex_type step = 0) const;

};

namespace MODEL_VERITAS_PA { extern void make_sharable(const PLAJA::Configuration& config); } // to hide internals of ModelZ3PA in main CEGAR


#endif //PLAJA_MODEL_VERITAS_PA_H
