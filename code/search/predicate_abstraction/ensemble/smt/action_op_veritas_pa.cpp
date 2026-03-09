//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//

#include "action_op_veritas_pa.h"
#include "../../../../parser/ast/expression/expression.h"
#include "../../../../parser/visitor/extern/variables_of.h"
#include "../../../../utils/utils.h"
#include "../../../smt_ensemble/visitor/extern/to_veritas_visitor.h"
#include "../../../smt_ensemble/constraints_in_veritas.h"
#include "../../../successor_generation/action_op.h"
#include "../../pa_states/abstract_state.h"
#include "../../successor_generation/action_op/action_op_pa.h"
#include "model_veritas_pa.h"

UpdateVeritasPA::UpdateVeritasPA(const Update& parent_, const ModelVeritasPA& model_):
    UpdateVeritas(parent_, model_) {
    step_predicates(true);
}

UpdateVeritasPA::~UpdateVeritasPA() = default;

//

const ModelVeritasPA& UpdateVeritasPA::get_model_veritas_pa() const { return PLAJA_UTILS::cast_ref<ModelVeritasPA>(model); }

//

void UpdateVeritasPA::generate_steps() {
    step_updates();
    step_predicates(false);
}

void UpdateVeritasPA::increment() {
    // set_additional_updating_vars(model_veritas);
    step_predicates(false);
}

//

void UpdateVeritasPA::step_predicates(bool reset) {

    const auto updated_state_indexes = collect_updated_state_indexes(false);
    const auto& model_pa = get_model_veritas_pa();
    const auto num_preds_in_model = model_pa.get_number_predicates();

    // cache affected preds
    std::vector<bool> affected_by_update(num_preds_in_model, false);
    for (PredicateIndex_type i = 0; i < num_preds_in_model; ++i) {
        const auto& predicate = model_pa.get_predicate(i);
        if (VARIABLES_OF::contains_state_indexes(predicate, updated_state_indexes, false, true)) { affected_by_update[i] = true; }
    }

    
    auto& predicates_in_veritas = predicatesInVeritas; // predicatesInVeritasPerStep[step];

    if (reset) { predicates_in_veritas.clear(); }

    // set updated state variables in mapping for predicates:
    auto target_predicate_state_indexes = model.get_state_indexes(0); // model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(1); // model.get_state_indexes(it.successor());
    for (const VariableIndex_type target_index: updated_state_indexes) { target_predicate_state_indexes.set(target_index, target_vars.to_veritas(target_index)); }

    // construct target predicates
    const auto l = model_pa.get_number_predicates();
    for (PredicateIndex_type i = predicates_in_veritas.size(); i < l; ++i) {
        const auto& predicate = model_pa.get_predicate(i);
        if (affected_by_update[i]) { predicates_in_veritas.push_back(VERITAS_IN_PLAJA::to_predicate_constraint(*predicate, target_predicate_state_indexes)); }
        else { predicates_in_veritas.emplace_back(nullptr); }
    }

    // }

}

void UpdateVeritasPA::add_target_predicate_state(const AbstractState& target, VERITAS_IN_PLAJA::QueryConstructable& query) const {
    PLAJA_ASSERT(target.has_entailment_information())
    // PLAJA_LOG("### TARGET STARTED");
    for (auto pred_it = target.init_pred_state_iterator(); !pred_it.end(); ++pred_it) {

        const auto* predicate_in_veritas = predicatesInVeritas[pred_it.predicate_index()].get(); // PerStep[step][pred_it.predicate_index()].get();

        if (pred_it.is_entailed() and (not predicate_in_veritas or not predicate_in_veritas->is_bound())) { continue; }

        bool truth_value;

        if (PLAJA_GLOBAL::lazyPA and not pred_it.is_set()) {

            if (pred_it.is_entailed()) { truth_value = pred_it.entailment_value(); }
            else { continue; }

        } else {

            PLAJA_ASSERT(pred_it.is_set())

            truth_value = pred_it.predicate_value();

        }

        PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or predicate_in_veritas)

        if (PLAJA_GLOBAL::lazyPA and not predicate_in_veritas) {
            /* Invariant predicate unknown in source but set in target.
             * Just add source variable predicate */

            const auto* pred_in_veritas_src = get_model_veritas_pa().get_predicate_in_veritas(pred_it.predicate_index());
            // pred_in_veritas_src->dump();
            if (truth_value) { pred_in_veritas_src->add_to_query(query); }
            else { pred_in_veritas_src->add_negation_to_query(query); }

        } else {
            // predicate_in_veritas->dump();
            if (truth_value) { predicate_in_veritas->add_to_query(query); }
            else { predicate_in_veritas->add_negation_to_query(query); }
        }
    }
    // PLAJA_LOG("### TARGET ENDED");
}

/**********************************************************************************************************************/


ActionOpVeritasPA::ActionOpVeritasPA(const ActionOp& parent_, const ModelVeritasPA& model_veritas):
    ActionOpVeritas(parent_, model_veritas, false) {
    updates.reserve(_concrete().size());
    for (auto update_it = parent_.updateIterator(); !update_it.end(); ++update_it) { updates.emplace_back(new UpdateVeritasPA(update_it.update(), model_veritas)); }
}

ActionOpVeritasPA::~ActionOpVeritasPA() = default;
