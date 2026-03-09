//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op_marabou_pa.h"
#include "../../../../parser/ast/expression/expression.h"
#include "../../../../parser/visitor/extern/variables_of.h"
#include "../../../../utils/utils.h"
#include "../../../smt_nn/visitor/extern/to_marabou_visitor.h"
#include "../../../smt_nn/constraints_in_marabou.h"
#include "../../../successor_generation/action_op.h"
#include "../../pa_states/abstract_state.h"
#include "../../successor_generation/action_op/action_op_pa.h"
#include "model_marabou_pa.h"

UpdateMarabouPA::UpdateMarabouPA(const Update& parent_, const ModelMarabouPA& model_):
    UpdateMarabou(parent_, model_) {
    step_predicates(true);
}

UpdateMarabouPA::~UpdateMarabouPA() = default;

//

const ModelMarabouPA& UpdateMarabouPA::get_model_marabou_pa() const { return PLAJA_UTILS::cast_ref<ModelMarabouPA>(model); }

//

void UpdateMarabouPA::generate_steps() {
    step_updates();
    step_predicates(false);
}

void UpdateMarabouPA::increment() {
    // set_additional_updating_vars(model_marabou);
    step_predicates(false);
}

//

void UpdateMarabouPA::step_predicates(bool reset) {

    const auto updated_state_indexes = collect_updated_state_indexes(false);
    const auto& model_pa = get_model_marabou_pa();
    const auto num_preds_in_model = model_pa.get_number_predicates();

    // cache affected preds
    std::vector<bool> affected_by_update(num_preds_in_model, false);
    for (PredicateIndex_type i = 0; i < num_preds_in_model; ++i) {
        const auto& predicate = model_pa.get_predicate(i);
        if (VARIABLES_OF::contains_state_indexes(predicate, updated_state_indexes, false, true)) { affected_by_update[i] = true; }
    }

    // predicatesInMarabouPerStep.reserve(model.get_max_path_len());
    // for (auto it = model.iterate_path(); !it.end(); ++it) {
    // auto step = it.step();
    // if (step >= predicatesInMarabouPerStep.size()) { predicatesInMarabouPerStep.emplace_back(); }
    auto& predicates_in_marabou = predicatesInMarabou; // predicatesInMarabouPerStep[step];

    if (reset) { predicates_in_marabou.clear(); }

    // set updated state variables in mapping for predicates:
    auto target_predicate_state_indexes = model.get_state_indexes(0); // model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(1); // model.get_state_indexes(it.successor());
    for (const VariableIndex_type target_index: updated_state_indexes) { target_predicate_state_indexes.set(target_index, target_vars.to_marabou(target_index)); }

    // construct target predicates
    const auto l = model_pa.get_number_predicates();
    for (PredicateIndex_type i = predicates_in_marabou.size(); i < l; ++i) {
        const auto& predicate = model_pa.get_predicate(i);
        if (affected_by_update[i]) { predicates_in_marabou.push_back(MARABOU_IN_PLAJA::to_predicate_constraint(*predicate, target_predicate_state_indexes)); }
        else { predicates_in_marabou.emplace_back(nullptr); }
    }

    // }

}

void UpdateMarabouPA::add_target_predicate_state(const AbstractState& target, MARABOU_IN_PLAJA::QueryConstructable& query) const {
    // add predicate constraints
    PLAJA_ASSERT(target.has_entailment_information())
    for (auto pred_it = target.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {

        const auto* predicate_in_marabou = predicatesInMarabou[pred_it.predicate_index()].get(); // PerStep[step][pred_it.predicate_index()].get();

        if (pred_it.is_entailed() and (not predicate_in_marabou or not predicate_in_marabou->is_bound())) { continue; }

        bool truth_value;

        if (PLAJA_GLOBAL::lazyPA and not pred_it.is_set()) {

            if (pred_it.is_entailed()) { truth_value = pred_it.entailment_value(); }
            else { continue; }

        } else {

            PLAJA_ASSERT(pred_it.is_set())

            truth_value = pred_it.predicate_value();

        }

        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or predicate_in_marabou)

        if (PLAJA_GLOBAL::lazyPA and not predicate_in_marabou) {
            /* Invariant predicate unknown in source but set in target.
             * Just add source variable predicate */

            const auto* pred_in_marabou_src = get_model_marabou_pa().get_predicate_in_marabou(pred_it.predicate_index());

            if (truth_value) { pred_in_marabou_src->add_to_query(query); }
            else { pred_in_marabou_src->add_negation_to_query(query); }

        } else {

            if (truth_value) { predicate_in_marabou->add_to_query(query); }
            else { predicate_in_marabou->add_negation_to_query(query); }

        }

    }
}

/**********************************************************************************************************************/


ActionOpMarabouPA::ActionOpMarabouPA(const ActionOp& parent_, const ModelMarabouPA& model_marabou):
    ActionOpMarabou(parent_, model_marabou, false) {
    updates.reserve(_concrete().size());
    for (auto update_it = parent_.updateIterator(); !update_it.end(); ++update_it) { updates.emplace_back(new UpdateMarabouPA(update_it.update(), model_marabou)); }
}

ActionOpMarabouPA::~ActionOpMarabouPA() = default;
