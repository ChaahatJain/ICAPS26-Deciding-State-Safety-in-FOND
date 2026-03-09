//
// This file is part of the PlaJA code base.
// Copyright (c) (2023 - 2024) Chaahat Jain.
//
#include "action_op_veritas.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../successor_generation/action_op.h"
#include "../constraints_in_veritas.h"
#include "../veritas_query.h"
#include "../model/model_veritas.h"
#include "../visitor/extern/to_veritas_visitor.h"
#include "../vars_in_veritas.h"
#include "../../../parser/ast/expression/expression.h"


UpdateVeritas::UpdateVeritas(const Update& update_, const ModelVeritas& model_):
    UpdateToVeritas(update_)
    , model(model_) {

    // src & target vars (deprecated)
    // set_additional_updating_vars(model_veritas);
    // for (VariableIndex_type state_index: collect_updated_state_indexes(include_locs)) { model_veritas.add_as_target(state_index); }

    PLAJA_ASSERT(model.get_max_num_step() >= 1)
    assignments = UpdateToVeritas::to_veritas(model.get_state_indexes(0), model.get_state_indexes(1), false, false);
}

UpdateVeritas::~UpdateVeritas() = default;

//

void UpdateVeritas::generate_steps() {}

// void UpdateVeritas::set_additional_updating_vars(ModelVeritas& model_veritas) { for (VariableIndex_type state_index: collect_updating_state_indexes()) { model_veritas.add_as_src(state_index); } }

void UpdateVeritas::increment() { /*set_additional_updating_vars(model);*/ }

//

void UpdateVeritas::add(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, bool do_copy, StepIndex_type step) const {
    // PLAJA_LOG("Update Started")
    // assignments->dump();
    const auto& src_vars = model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(step + 1);

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        std::unordered_set<VariableIndex_type> updated_locs;

        for (auto it = update.locationIterator(); !it.end(); ++it) {
            updated_locs.insert(it.loc());
            query.tighten_lower_bound(target_vars.to_veritas(it.loc()), it.dest());
            query.tighten_upper_bound(target_vars.to_veritas(it.loc()), it.dest());
        }

        if (do_copy) {

            for (auto it = model.get_model_info().locationIndexIterator(); !it.end(); ++it) {

                if (not updated_locs.count(it.state_index())) {
                    const auto veritas_src = src_vars.to_veritas(it.state_index());
                    const auto veritas_target = target_vars.to_veritas(it.state_index());
                    const auto lb = query.get_lower_bound(veritas_src);
                    const auto ub = query.get_upper_bound(veritas_src);
                    if (ub == lb) {
                        query.tighten_lower_bound(veritas_target, ub);
                        query.tighten_upper_bound(veritas_target, lb);
                    } else {
                        VERITAS_IN_PLAJA::to_copy_constraint(model.get_context(), veritas_target, veritas_src)->move_to_query(query, true);
                    }
                }

            }

        }
    }

    if (step > 0) {

        // auto mapping = model.get_state_indexes(0).compute_substitution(model.get_state_indexes(step));
        // model.get_state_indexes(step).compute_substitution(mapping, model.get_state_indexes(step + 1));
        // assignments->to_substitute(mapping)->move_to_query(query);

        UpdateToVeritas::to_veritas(src_vars, target_vars, false, false)->move_to_query(query, true);

    } else {

        assignments->add_to_query(query, true);

    }

    if (do_copy) {

        const auto updated_vars = collect_updated_state_indexes(false);

        for (auto it = model.get_model_info().stateIndexIterator(false); !it.end(); ++it) {

            if (not updated_vars.count(it.state_index())) {
                const auto veritas_src = src_vars.to_veritas(it.state_index());
                const auto veritas_target = target_vars.to_veritas(it.state_index());
                const auto lb = query.get_lower_bound(veritas_src);
                const auto ub = query.get_upper_bound(veritas_src);
                if (ub == lb) {
                    query.tighten_lower_bound(veritas_target, ub);
                    query.tighten_upper_bound(veritas_target, lb);
                } else {
                    VERITAS_IN_PLAJA::to_copy_constraint(model.get_context(), veritas_target, veritas_src)->move_to_query(query, true);
                }
            }

        }

    }
    // PLAJA_LOG("Update Veritas END")

}

std::unique_ptr<ConjunctionInVeritas> UpdateVeritas::to_veritas(bool do_locs, bool do_copy, StepIndex_type step) const {
    PLAJA_ASSERT(model.get_max_num_step() >= step + 1)
    const auto& src_vars = model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(step + 1);

    auto assignment_conjunction = std::make_unique<ConjunctionInVeritas>(model.get_context());

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        to_veritas_loc(src_vars, target_vars, do_copy)->move_to_conjunction(*assignment_conjunction);

    }

    if (step > 0) {

        // auto mapping = model.get_state_indexes(0).compute_substitution(model.get_state_indexes(step));
        // model.get_state_indexes(step).compute_substitution(mapping, model.get_state_indexes(step + 1));
        // assignments->to_substitute(mapping)->move_to_conjunction(*assignment_conjunction);

        UpdateToVeritas::to_veritas(src_vars, target_vars, false, false)->move_to_conjunction(*assignment_conjunction);

    } else {

        assignments->add_to_conjunction(*assignment_conjunction);

    }

    if (do_copy) { to_veritas_copy(src_vars, target_vars, false)->move_to_conjunction(*assignment_conjunction); }

    return assignment_conjunction;
}

/**********************************************************************************************************************/

ActionOpVeritas::ActionOpVeritas(const ActionOp& action_op, const ModelVeritas& model_, bool compute_updates):
    ActionOpToVeritas(action_op, false)
    , model(model_)
    , guards(ActionOpToVeritas::guard_to_veritas(model.get_src_in_veritas(), false))
    // , guardNegations(guards->compute_negation()) 
    {

    // set guard vars
    // set_additional_guard_vars(model_veritas);

    // updates
    updates.reserve(_concrete().size());
    if (compute_updates) { for (auto update_it = _concrete().updateIterator(); !update_it.end(); ++update_it) { updates.push_back(std::make_unique<UpdateVeritas>(update_it.update(), model)); } }
    if(not model.no_filter()) {
        if (model.indicator_filter()) {
            auto policy = model.get_policy_in_query();
            applicabilityTrees = guards->add_operator_applicability(_op_id(), policy); 
        } else if (model.naive_filter()) {
            PLAJA_LOG("Naive applicability filtering must be loaded in rather than computed.")
        }
    }
}

ActionOpVeritas::~ActionOpVeritas() = default;

//

void ActionOpVeritas::generate_steps() { for (auto& update: updates) { PLAJA_UTILS::cast_ref<UpdateVeritas>(*update).generate_steps(); } }

// void ActionOpVeritas::set_additional_guard_vars(ModelVeritas& model_veritas) { for (VariableIndex_type state_index: _concrete().collect_guard_state_indexes()) { model_veritas.add_as_src(state_index); } }

void ActionOpVeritas::increment() {
    // set_additional_guard_vars(model_veritas);
    for (auto& update: updates) { PLAJA_UTILS::cast_ref<UpdateVeritas>(*update).increment(); }
}

veritas::AddTree ActionOpVeritas::get_applicability_tree() const { 
    return applicabilityTrees;
}

//

bool ActionOpVeritas::add_guard(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {
    // PLAJA_LOG("### ACTION STARTED")
    // guards->dump();
    const auto& src_vars = model.get_state_indexes(step);

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        for (auto it_loc = actionOp.locationIterator(); !it_loc.end(); ++it_loc) {

            query.tighten_lower_bound(src_vars.to_veritas(it_loc.loc()), it_loc.src());
            query.tighten_upper_bound(src_vars.to_veritas(it_loc.loc()), it_loc.src());

        }
    } else if (guards->empty()) { return false; } // Special case.

    if (step > 0) { ActionOpToVeritas::guard_to_veritas(src_vars, false)->move_to_query(query); }
    else { guards->add_to_query(query); }
    // PLAJA_LOG("ACTION ENDED")
    return true;
}

bool ActionOpVeritas::add_guard_negation(VERITAS_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {

    if (step > 0 or do_locs) {

        ActionOpToVeritas::guard_to_veritas(model.get_state_indexes(step), do_locs)->move_to_query(query);

    } else {

        guardNegations->add_to_query(query);

    }

    return not guards->empty() or do_locs;
}

std::unique_ptr<ConjunctionInVeritas> ActionOpVeritas::guard_to_veritas(bool do_locs, StepIndex_type step) const {

    if (step > 0) {

        return ActionOpToVeritas::guard_to_veritas(model.get_state_indexes(step), do_locs);

    } else {

        if (do_locs) {
            PLAJA_ASSERT(not model.ignore_locs())

            auto guard_in_veritas = guard_to_veritas_loc(model.get_state_indexes(step));

            guards->add_to_conjunction(*guard_in_veritas);

            return guard_in_veritas;

        } else {

            return std::make_unique<ConjunctionInVeritas>(*guards);

        }

    }

}

std::unique_ptr<VeritasConstraint> ActionOpVeritas::guard_negation_to_veritas(bool do_locs, StepIndex_type step) const {

    if (step > 0 or do_locs) {

        return ActionOpToVeritas::guard_to_veritas(model.get_state_indexes(step), do_locs)->move_to_negation();

    } else {

        return guardNegations->copy();

    }

}

