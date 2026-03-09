//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "action_op_marabou.h"
#include "../../../utils/utils.h"
#include "../../information/model_information.h"
#include "../../successor_generation/action_op.h"
#include "../constraints_in_marabou.h"
#include "../marabou_query.h"
#include "../model/model_marabou.h"
#include "../visitor/extern/to_marabou_visitor.h"

UpdateMarabou::UpdateMarabou(const Update& update_, const ModelMarabou& model_):
    UpdateToMarabou(update_)
    , model(model_) {

    // src & target vars (deprecated)
    // set_additional_updating_vars(model_marabou);
    // for (VariableIndex_type state_index: collect_updated_state_indexes(include_locs)) { model_marabou.add_as_target(state_index); }

    PLAJA_ASSERT(model.get_max_num_step() >= 1)
    assignments = UpdateToMarabou::to_marabou(model.get_state_indexes(0), model.get_state_indexes(1), false, false);
}

UpdateMarabou::~UpdateMarabou() = default;

//

void UpdateMarabou::generate_steps() {}

// void UpdateMarabou::set_additional_updating_vars(ModelMarabou& model_marabou) { for (VariableIndex_type state_index: collect_updating_state_indexes()) { model_marabou.add_as_src(state_index); } }

void UpdateMarabou::increment() { /*set_additional_updating_vars(model);*/ }

//

void UpdateMarabou::add(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, bool do_copy, StepIndex_type step) const {

    const auto& src_vars = model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(step + 1);

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        std::unordered_set<VariableIndex_type> updated_locs;

        for (auto it = update.locationIterator(); !it.end(); ++it) {
            updated_locs.insert(it.loc());
            query.tighten_lower_bound(target_vars.to_marabou(it.loc()), it.dest());
            query.tighten_upper_bound(target_vars.to_marabou(it.loc()), it.dest());
        }

        if (do_copy) {

            for (auto it = model.get_model_info().locationIndexIterator(); !it.end(); ++it) {

                if (not updated_locs.count(it.state_index())) {
                    const auto marabou_src = src_vars.to_marabou(it.state_index());
                    const auto marabou_target = target_vars.to_marabou(it.state_index());
                    const auto lb = query.get_lower_bound(marabou_src);
                    const auto ub = query.get_upper_bound(marabou_src);
                    if (ub == lb) {
                        query.tighten_lower_bound(marabou_target, ub);
                        query.tighten_upper_bound(marabou_target, lb);
                    } else {
                        MARABOU_IN_PLAJA::to_copy_constraint(model.get_context(), marabou_target, marabou_src)->move_to_query(query);
                    }
                }

            }

        }
    }

    if (step > 0) {

        // auto mapping = model.get_state_indexes(0).compute_substitution(model.get_state_indexes(step));
        // model.get_state_indexes(step).compute_substitution(mapping, model.get_state_indexes(step + 1));
        // assignments->to_substitute(mapping)->move_to_query(query);

        UpdateToMarabou::to_marabou(src_vars, target_vars, false, false)->move_to_query(query);

    } else {

        assignments->add_to_query(query);

    }

    if (do_copy) {

        const auto updated_vars = collect_updated_state_indexes(false);

        for (auto it = model.get_model_info().stateIndexIterator(false); !it.end(); ++it) {

            if (not updated_vars.count(it.state_index())) {
                const auto marabou_src = src_vars.to_marabou(it.state_index());
                const auto marabou_target = target_vars.to_marabou(it.state_index());
                const auto lb = query.get_lower_bound(marabou_src);
                const auto ub = query.get_upper_bound(marabou_src);
                if (ub == lb) {
                    query.tighten_lower_bound(marabou_target, ub);
                    query.tighten_upper_bound(marabou_target, lb);
                } else {
                    MARABOU_IN_PLAJA::to_copy_constraint(model.get_context(), marabou_target, marabou_src)->move_to_query(query);
                }
            }

        }

    }

}

std::unique_ptr<ConjunctionInMarabou> UpdateMarabou::to_marabou(bool do_locs, bool do_copy, StepIndex_type step) const {
    PLAJA_ASSERT(model.get_max_num_step() >= step + 1)
    const auto& src_vars = model.get_state_indexes(step);
    const auto& target_vars = model.get_state_indexes(step + 1);

    auto assignment_conjunction = std::make_unique<ConjunctionInMarabou>(model.get_context());

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        to_marabou_loc(src_vars, target_vars, do_copy)->move_to_conjunction(*assignment_conjunction);

    }

    if (step > 0) {

        // auto mapping = model.get_state_indexes(0).compute_substitution(model.get_state_indexes(step));
        // model.get_state_indexes(step).compute_substitution(mapping, model.get_state_indexes(step + 1));
        // assignments->to_substitute(mapping)->move_to_conjunction(*assignment_conjunction);

        UpdateToMarabou::to_marabou(src_vars, target_vars, false, false)->move_to_conjunction(*assignment_conjunction);

    } else {

        assignments->add_to_conjunction(*assignment_conjunction);

    }

    if (do_copy) { to_marabou_copy(src_vars, target_vars, false)->move_to_conjunction(*assignment_conjunction); }

    return assignment_conjunction;
}

/**********************************************************************************************************************/

ActionOpMarabou::ActionOpMarabou(const ActionOp& action_op, const ModelMarabou& model_, bool compute_updates):
    ActionOpToMarabou(action_op, false)
    , model(model_)
    , guards(ActionOpToMarabou::guard_to_marabou(model.get_src_in_marabou(), false))
    , guardNegations(guards->compute_negation()) {

    // set guard vars
    // set_additional_guard_vars(model_marabou);

    // updates
    updates.reserve(_concrete().size());
    if (compute_updates) { for (auto update_it = _concrete().updateIterator(); !update_it.end(); ++update_it) { updates.push_back(std::make_unique<UpdateMarabou>(update_it.update(), model)); } }

}

ActionOpMarabou::~ActionOpMarabou() = default;

//

void ActionOpMarabou::generate_steps() { for (auto& update: updates) { PLAJA_UTILS::cast_ref<UpdateMarabou>(*update).generate_steps(); } }

// void ActionOpMarabou::set_additional_guard_vars(ModelMarabou& model_marabou) { for (VariableIndex_type state_index: _concrete().collect_guard_state_indexes()) { model_marabou.add_as_src(state_index); } }

void ActionOpMarabou::increment() {
    // set_additional_guard_vars(model_marabou);
    for (auto& update: updates) { PLAJA_UTILS::cast_ref<UpdateMarabou>(*update).increment(); }
}

/**********************************************************************************************************************/

bool ActionOpMarabou::add_guard(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {
    const auto& src_vars = model.get_state_indexes(step);

    if (do_locs) {
        PLAJA_ASSERT(not model.ignore_locs())

        for (auto it_loc = actionOp.locationIterator(); !it_loc.end(); ++it_loc) {

            query.tighten_lower_bound(src_vars.to_marabou(it_loc.loc()), it_loc.src());
            query.tighten_upper_bound(src_vars.to_marabou(it_loc.loc()), it_loc.src());

        }
    } else if (guards->empty()) { return false; } // Special case.

    if (step > 0) { ActionOpToMarabou::guard_to_marabou(src_vars, false)->move_to_query(query); }
    else { guards->add_to_query(query); }

    return true;
}

bool ActionOpMarabou::add_guard_negation(MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {

    if (step > 0 or do_locs) {

        ActionOpToMarabou::guard_to_marabou(model.get_state_indexes(step), do_locs)->move_to_query(query);

    } else {

        guardNegations->add_to_query(query);

    }

    return not guards->empty() or do_locs;
}

std::unique_ptr<ConjunctionInMarabou> ActionOpMarabou::guard_to_marabou(bool do_locs, StepIndex_type step) const {

    if (step > 0) {

        return ActionOpToMarabou::guard_to_marabou(model.get_state_indexes(step), do_locs);

    } else {

        if (do_locs) {
            PLAJA_ASSERT(not model.ignore_locs())

            auto guard_in_marabou = guard_to_marabou_loc(model.get_state_indexes(step));

            guards->add_to_conjunction(*guard_in_marabou);

            return guard_in_marabou;

        } else {

            return std::make_unique<ConjunctionInMarabou>(*guards);

        }

    }

}

std::unique_ptr<MarabouConstraint> ActionOpMarabou::guard_negation_to_marabou(bool do_locs, StepIndex_type step) const {

    if (step > 0 or do_locs) {

        return ActionOpToMarabou::guard_to_marabou(model.get_state_indexes(step), do_locs)->move_to_negation();

    } else {

        return guardNegations->copy();

    }

}

/**********************************************************************************************************************/

std::pair<std::unique_ptr<MarabouConstraint>, bool> ActionOpMarabou::guard_to_marabou(const MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {

    DisjunctionConstraintBase::EntailmentValue entailment(DisjunctionConstraintBase::Unknown);
    std::unique_ptr<MarabouConstraint> optimized_guard(nullptr);

    if (step > 0 or do_locs) {
        optimized_guard = ActionOpToMarabou::guard_to_marabou(model.get_state_indexes(step), do_locs);
        std::unique_ptr<MarabouConstraint> optimized_constraint(nullptr);
        entailment = optimized_guard->optimize_for_query(query, optimized_constraint);

        if (optimized_constraint) {
            PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_ptr_type<DisjunctionInMarabou>(optimized_guard.get()))
            PLAJA_ASSERT(entailment == DisjunctionConstraintBase::Unknown)
            optimized_guard = std::move(optimized_constraint);
        }

    } else {
        optimized_guard = guards->optimize_for_query(query, entailment);
    }

    switch (entailment) {
        case DisjunctionConstraintBase::Unknown: { return { std::move(optimized_guard), false }; }
        case DisjunctionConstraintBase::Infeasible: { return { nullptr, false }; }
        case DisjunctionConstraintBase::Entailed: { return { nullptr, true }; }
        default: { PLAJA_ABORT }
    }

}

std::pair<std::unique_ptr<MarabouConstraint>, bool> ActionOpMarabou::guard_negation_to_marabou(const MARABOU_IN_PLAJA::QueryConstructable& query, bool do_locs, StepIndex_type step) const {

    DisjunctionConstraintBase::EntailmentValue entailment(DisjunctionConstraintBase::Unknown);
    std::unique_ptr<MarabouConstraint> optimized_negation(nullptr);

    if (step > 0 or do_locs) {
        optimized_negation = ActionOpToMarabou::guard_to_marabou(model.get_state_indexes(step), do_locs)->move_to_negation();
        std::unique_ptr<MarabouConstraint> optimized_constraint(nullptr);
        entailment = optimized_negation->optimize_for_query(query, optimized_constraint);

        if (optimized_constraint) {
            PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_ptr_type<DisjunctionInMarabou>(optimized_negation.get()))
            PLAJA_ASSERT(entailment == DisjunctionConstraintBase::Unknown)
            optimized_negation = std::move(optimized_constraint);
        }

    } else {
        optimized_negation = guardNegations->optimize_for_query(query, entailment);
    }

    switch (entailment) {
        case DisjunctionConstraintBase::Unknown: { return { std::move(optimized_negation), false }; }
        case DisjunctionConstraintBase::Infeasible: { return { nullptr, false }; }
        case DisjunctionConstraintBase::Entailed: { return { nullptr, true }; }
        default: { PLAJA_ABORT }
    }

}
