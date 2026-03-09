//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_marabou_pa.h"
#include "../../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../../utils/utils.h"
#include "../../../factories/configuration.h"
#include "../../../smt_nn/visitor/extern/to_marabou_visitor.h"
#include "../../../smt_nn/constraints_in_marabou.h"
#include "../../../smt_nn/marabou_query.h"
#include "../../pa_states/abstract_state.h"
#include "../smt/successor_generator_marabou_pa.h"

ModelMarabouPA::ModelMarabouPA(const PLAJA::Configuration& config, const PredicatesExpression* predicates_):
    ModelMarabou(config, false) {

    if (predicates_) {
        PLAJA_ASSERT(not _predicates())
        predicates = predicates_;
    }

    PLAJA_ASSERT(_predicates())

    // predicate dependent
    step_predicates(true);

    successorGenerator = std::make_unique<MARABOU_IN_PLAJA::SuccessorGeneratorPA>(get_successor_generator(), *this);
    PLAJA_ASSERT(not outputConstraints) // First need successor generator.
    init_output_constraints();

}

ModelMarabouPA::~ModelMarabouPA() = default;

//

void ModelMarabouPA::generate_steps(StepIndex_type max_step) {
    ModelMarabou::generate_steps(max_step);
    step_predicates(false);
}

void ModelMarabouPA::increment() {
    PLAJA_ASSERT(get_num_predicates_marabou() <= predicates->get_number_predicates()) // number of predicates should have increased (or already being incremented to current count)

    step_predicates(false);
    successorGenerator->increment();
}

void ModelMarabouPA::step_predicates(bool reset) {

    const auto num_preds = predicates->get_number_predicates();
    predicatesInMarabouPerStep.reserve(get_max_num_step());

    for (auto it = iterate_steps(); !it.end(); ++it) {
        const auto step = it.step();

        if (step >= predicatesInMarabouPerStep.size()) { predicatesInMarabouPerStep.emplace_back(); }
        auto& predicates_in_marabou = predicatesInMarabouPerStep[step];

        if (reset) { predicates_in_marabou.clear(); }

        PLAJA_ASSERT(predicates_in_marabou.size() <= num_preds) // incremental usage: number of predicates should only increase (or be equal when already incremented)

        predicates_in_marabou.reserve(num_preds);

        for (PredicateIndex_type index = predicates_in_marabou.size(); index < num_preds; ++index) { // incremental usage: continue after existing bounds
            const auto* predicate = predicates->get_predicate(index);
            // for (VariableIndex_type state_index: VARIABLES_OF::state_indexes_of(predicate, false, false)) { add_as_src(state_index); }
            predicates_in_marabou.push_back(MARABOU_IN_PLAJA::to_predicate_constraint(*predicate, get_state_indexes(step)));
        }

    }

}

//

const ActionOpMarabouPA& ModelMarabouPA::get_action_op_pa(ActionOpID_type op_id) const { return PLAJA_UTILS::cast_ref<MARABOU_IN_PLAJA::SuccessorGeneratorPA>(*successorGenerator).get_action_op_pa(op_id); }

//

void ModelMarabouPA::add_to(const PaStateBase& pa_state, MARABOU_IN_PLAJA::QueryConstructable& query, StepIndex_type step) const {

    // locations
    PLAJA_ASSERT(get_input_locations().empty() or not ignore_locs())
    PLAJA_ASSERT_EXPECT(pa_state.get_size_locs() == 1 or not ignore_locs())
    if (not ignore_locs()) {
        const auto& vars_marabou = get_state_indexes(step);

        for (auto it = pa_state.init_loc_state_it(); !it.end(); ++it) {
            // for (const VariableIndex_type state_index: _input_locations()) {
            const auto loc_val = it.location_value();
            const auto marabou_var = vars_marabou.to_marabou(it.location_index());

            query.tighten_lower_bound(marabou_var, loc_val);
            query.tighten_upper_bound(marabou_var, loc_val);
        }

    }

    // add predicate constraints
    const auto& predicates_in_marabou = get_predicates_in_marabou(step);
    PLAJA_ASSERT(not pa_state.has_entailment_information())
    for (auto pred_it = pa_state.init_pred_state_it(); !pred_it.end(); ++pred_it) {

        if constexpr (PLAJA_GLOBAL::lazyPA) { if (not pred_it.is_set()) { continue; } }

        if (pred_it.predicate_value()) { predicates_in_marabou[pred_it.predicate_index()]->add_to_query(query); }
        else { predicates_in_marabou[pred_it.predicate_index()]->add_negation_to_query(query); }

    }

}

//

void MODEL_MARABOU_PA::make_sharable(const PLAJA::Configuration& config) {
    PLAJA_ASSERT(not config.has_sharable(PLAJA::SharableKey::MODEL_MARABOU))
    config.set_sharable(PLAJA::SharableKey::MODEL_MARABOU, std::shared_ptr<ModelMarabou>(new ModelMarabouPA(config)));
}