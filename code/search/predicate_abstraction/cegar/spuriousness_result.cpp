//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <unordered_map>
#include "spuriousness_result.h"
#include "../../../option_parser/option_parser.h"
#include "../../../parser/ast/expression/lvalue_expression.h"
#include "../../../parser/ast/assignment.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../../parser/visitor/variable_substitution.h"
#include "../../../stats/stats_base.h"
#include "../../successor_generation/action_op.h"
#include "../../states/state_base.h"
#include "../../smt/solver/smt_solver_z3.h"
#include "../../smt/vars_in_z3.h"
#include "../smt/model_z3_pa.h"
#include "../pa_states/abstract_path.h"
#include "../pa_states/abstract_state.h"
#include "../solution_checker_instance_pa.h"
#include "predicates_structure.h"

struct PathWP final {
    friend class SpuriousnessResult;

private:
    std::vector<std::list<std::unique_ptr<Expression>>> predicatesPerStep;
    std::vector<std::unordered_map<VariableID_type, const Expression*>> substitutionPerStep;
    std::vector<std::unordered_map<VariableID_type, const Assignment*>> subNonDetPerStep;

    inline void set_path_size(std::size_t num_steps) { predicatesPerStep.resize(num_steps); }

    inline std::list<std::unique_ptr<Expression>>& wp_per_step(StepIndex_type step) {
        PLAJA_ASSERT(step < predicatesPerStep.size())
        return predicatesPerStep[step];
    }

    inline void add_predicate(std::unique_ptr<Expression>&& pred, StepIndex_type step) { wp_per_step(step).push_back(std::move(pred)); }

    inline void clear(StepIndex_type step) { wp_per_step(step).clear(); }

    inline void clear() { for (auto& preds_per_step: predicatesPerStep) { preds_per_step.clear(); } }

    inline void set_substitution(const AbstractPath& path) {

        if (predicatesPerStep.empty()) { return; }

        substitutionPerStep.clear();
        substitutionPerStep.reserve(predicatesPerStep.size() - 1);
        for (auto it_path = path.init_prefix_iterator(predicatesPerStep.size() - 1); !it_path.end(); ++it_path) {

            const auto& action_op = it_path.retrieve_op();

            PLAJA_ASSERT(action_op.get_update(it_path.get_update_index()).get_num_sequences() <= 1)

            substitutionPerStep.emplace_back();
            auto& substitution = substitutionPerStep.back();
            subNonDetPerStep.emplace_back();
            auto& sub_non_det = subNonDetPerStep.back();

            for (auto it_assignment = action_op.get_update(it_path.get_update_index()).assignmentIterator(0); !it_assignment.end(); ++it_assignment) {
                const auto* assignment = it_assignment();
                const auto* ref = assignment->get_ref();
                if (ref->get_array_index()) {
                    PLAJA_LOG("Substituting array variables is not supported.");
                    PLAJA_ABORT
                }
                if (assignment->is_deterministic()) { substitution.emplace(ref->get_variable_id(), assignment->get_value()); }
                else { sub_non_det.emplace(ref->get_variable_id(), assignment); }
            }

        }

    }

    inline std::unordered_map<VariableID_type, const Expression*>& get_substitution(StepIndex_type step) {
        PLAJA_ASSERT(step < substitutionPerStep.size())
        return substitutionPerStep[step];
    }

    inline std::unordered_map<VariableID_type, const Assignment*>& get_sub_non_det(StepIndex_type step) {
        PLAJA_ASSERT(step < subNonDetPerStep.size())
        return subNonDetPerStep[step];
    }

public:
    PathWP() = default;
    ~PathWP() = default;
    DELETE_CONSTRUCTOR(PathWP)

    [[nodiscard]] inline std::size_t get_path_size() const { return predicatesPerStep.size(); }

};

/**********************************************************************************************************************/

SpuriousnessResult::SpuriousnessResult(const PredicatesStructure& predicates, std::unique_ptr<SolutionCheckerInstancePaPath>&& path_instance):
    predicates(&predicates)
    , pathInstance(std::move(path_instance))
    , path(&pathInstance->get_path())
    , targetStep(0)
    , spuriousnessCause(SpuriousnessCause::None)
    CONSTRUCT_IF_LAZY_PA(doLocalRefinement(false))
    , pathWpIn(new PathWP())
    , pathWpOut(new PathWP())
    , globalPredicatesAdded(0)
    , localPredicatesAdded(0) {
}

SpuriousnessResult::~SpuriousnessResult() = default;

/**********************************************************************************************************************/

std::unordered_set<VariableID_type> SpuriousnessResult::collect_updated_vars() {
    std::unordered_set<VariableID_type> updated_vars;

    for (auto it = path->init_prefix_iterator(targetStep); !it.end(); ++it) {
        const auto& action_op = it.retrieve_op();
        PLAJA_ASSERT(action_op.get_update(it.get_update_index()).get_num_sequences() <= 1)
        const auto updated_vars_up = action_op.get_update(it.get_update_index()).collect_updated_vars(0);
        updated_vars.insert(updated_vars_up.begin(), updated_vars_up.end());
    }

    return updated_vars;
}

std::unordered_set<VariableIndex_type> SpuriousnessResult::collect_updated_state_indexes(bool include_locs) {
    std::unordered_set<VariableIndex_type> updated_state_indexes;

    for (auto it = path->init_prefix_iterator(targetStep); !it.end(); ++it) {
        const auto& action_op = it.retrieve_op();
        PLAJA_ASSERT(action_op.get_update(it.get_update_index()).get_num_sequences() <= 1)
        const auto updated_indexes_up = action_op.get_update(it.get_update_index()).collect_updated_state_indexes(0, include_locs);
        updated_state_indexes.insert(updated_indexes_up.begin(), updated_indexes_up.end());
    }

    return updated_state_indexes;
}

// TODO Might want to have option to constraint all abstract states.
std::list<VariableIndex_type> SpuriousnessResult::compute_entailments(const StateBase& state, bool complete) {
    std::list<VariableIndex_type> entailments;

    const auto& model_z3 = predicates->prepare_model_z3(complete ? path->get_size() : targetStep);

    Z3_IN_PLAJA::SMTSolver solver(model_z3.share_context(), predicates->share_stats());

    model_z3.add_start(solver, true);
    model_z3.add_to(path->get_pa_source_state(), solver, 0);

    for (auto it = complete ? path->init_path_iterator() : path->init_prefix_iterator(targetStep); !it.end(); ++it) {
        const auto& action_op = it.retrieve_op();
        model_z3.add_action_op(solver, action_op._op_id(), it.get_update_index(), not model_z3.ignore_locs(), true, it.get_step());
        PLAJA_ASSERT(complete or it.get_step() < get_spurious_prefix_length())
    }

    if (complete) { model_z3.add_reach(solver, path->get_target_step()); }

    const auto& state_vars = model_z3.get_state_vars(targetStep);
    for (VariableIndex_type state_index = 0; state_index < state.size(); ++state_index) {
        if (not state_vars.exists_index(state_index)) {
            PLAJA_ASSERT(state_vars.is_loc(state_index))
            continue;
        }
        STMT_IF_DEBUG(solver.push())
        STMT_IF_DEBUG(solver.add(state_vars.encode_state_value(state, state_index)))
        PLAJA_ASSERT(solver.check_pop())
        solver.push();
        solver.add(!state_vars.encode_state_value(state, state_index));
        if (not solver.check_pop()) { entailments.push_back(state_index); }
    }

    return entailments;
}

std::unordered_set<VariableIndex_type> SpuriousnessResult::compute_entailments_set(const StateBase& state, bool complete) {
    const auto entailments = compute_entailments(state, complete);
    return { entailments.cbegin(), entailments.cend() };
}

/**********************************************************************************************************************/

std::unique_ptr<SolutionCheckerInstancePaPath> SpuriousnessResult::retrieve_path_instance() { return std::move(pathInstance); }

void SpuriousnessResult::set_concretization(std::vector<std::unique_ptr<StateValues>>&& concretization) {
    PLAJA_ASSERT(pathInstance)
    pathInstance->set_solution(std::move(concretization));
    PLAJA_ASSERT(pathInstance->check())
}

void SpuriousnessResult::set_spurious(SpuriousnessCause cause) {
    // pathInstance->invalidate();
    spuriousnessCause = cause;

    pathWpIn->set_path_size(get_prefix_length() + 1);
    pathWpOut->set_path_size(pathWpIn->get_path_size());
    PLAJA_ASSERT(pathWpOut->get_path_size() > 0)

    switch (spuriousnessCause) {
        case None: { PLAJA_ABORT }
        case Concretization: {
            PLAJA_LOG("Spurious path: Concrete State not in Abstract State")
            STMT_IF_LAZY_PA(doLocalRefinement = predicates->do_lazy_pa_state();)
            break;
        }
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
            case Terminal: {
                PLAJA_LOG("Spurious path: Terminal state!")
                STMT_IF_LAZY_PA(PLAJA_ABORT)
                break;
            }
#endif
        case GuardApp: {
            PLAJA_LOG("Spurious path: Guard is not applicable!")
            STMT_IF_LAZY_PA(doLocalRefinement = predicates->do_lazy_pa_app();)
            break;
        }
        case ActionSel: {
            PLAJA_LOG("Spurious path: Action is not selected!")
            STMT_IF_LAZY_PA(doLocalRefinement = predicates->do_lazy_pa_sel();)
            break;
        }
        case Target: {
            PLAJA_LOG("Spurious path: Target not reached!")
            STMT_IF_LAZY_PA(doLocalRefinement = predicates->do_lazy_pa_target();)
            break;
        }
        default: { PLAJA_ABORT }
    }

    pathWpIn->set_substitution(*path);

    // cache predicates
    if (PLAJA_GLOBAL::lazyPA and do_local_refinement()) {
        predicateSet.reserve(targetStep + 1);
        for (StepIndex_type step = 0; step <= targetStep; ++step) {
            predicateSet.emplace_back(ExpressionSet {}, predicates->collect_predicates(path->get_pa_state(step)));
        }
    }

    globalPredicatesSet = { ExpressionSet {}, PLAJA_GLOBAL::lazyPA ? predicates->collect_global_predicates() : ExpressionSet {} };

}

ExpressionSet& SpuriousnessResult::get_new_predicates(StepIndex_type step) {
    if (PLAJA_GLOBAL::lazyPA and do_local_refinement()) {
        PLAJA_ASSERT(step < predicateSet.size())
        return predicateSet[step].first;
    } else {
        PLAJA_ASSERT(predicateSet.empty())
        return globalPredicatesSet.first;
    }
}

const ExpressionSet* SpuriousnessResult::get_used_predicates(StepIndex_type step) const {
    if constexpr (PLAJA_GLOBAL::lazyPA) {
        PLAJA_ASSERT(not do_local_refinement() or step < predicateSet.size())
        return &(do_local_refinement() ? predicateSet[step] : globalPredicatesSet).second;
    } else {
        return nullptr;
    }
}

const PaStateBase& SpuriousnessResult::get_pa_state(StepIndex_type step) const { return path->get_pa_state(step); }

const PaStateBase* SpuriousnessResult::get_pa_state_if_local(StepIndex_type step) const {
    if (PLAJA_GLOBAL::lazyPA and do_local_refinement()) { return &path->get_pa_state(step); }
    else { return nullptr; }
}

/**********************************************************************************************************************/

void SpuriousnessResult::add_refinement_predicate(std::unique_ptr<Expression>&& refinement_pred, StepIndex_type step_index) {
    PLAJA_ASSERT(refinement_pred.get())
    PLAJA_ASSERT(TO_NORMALFORM::normalize_and_split(*refinement_pred, true).size() == 1); // important: we use the expression (*not* the unique_ptr) reference in the assertion
    PLAJA_ASSERT(step_index <= get_prefix_length())

    if (predicates->do_compute_wp()) { pathWpIn->add_predicate(std::move(refinement_pred), step_index); }
    else { add_wp_out(std::move(refinement_pred), step_index); }
}

void SpuriousnessResult::add_refinement_predicate(PredicateIndex_type predicate_index, StepIndex_type step_index) {
    const auto* predicate = predicates->operator[](predicate_index);

    PLAJA_ASSERT(predicate)
    PLAJA_ASSERT(TO_NORMALFORM::normalize_and_split(*predicate, true).size() == 1);
    PLAJA_ASSERT(step_index <= get_prefix_length())

    pathWpIn->add_predicate(predicate->deepCopy_Exp(), step_index); // always do wp computation for existing predicates
}

void SpuriousnessResult::add_refinement_predicate_wp(std::unique_ptr<Expression>&& refinement_pred) {
    add_refinement_predicate(std::move(refinement_pred), get_spurious_prefix_length());
    compute_wp();
}

void SpuriousnessResult::add_refinement_predicate(std::unique_ptr<Expression>&& refinement_pred) {
    PLAJA_ASSERT(refinement_pred.get())
    PLAJA_ASSERT_EXPECT(TO_NORMALFORM::normalize_and_split(*refinement_pred, true).size() == 1)

    auto& predicates_set = globalPredicatesSet.first;

    auto prepared_splits = predicates->prepare_predicate(std::move(refinement_pred), PLAJA_GLOBAL::lazyPA ? &globalPredicatesSet.second : nullptr, nullptr);

    for (auto& prepared_split: prepared_splits) {

        auto rlt = predicates_set.insert(prepared_split.get()); // local cache

        if (not rlt.second) { continue; }

        const auto* prepared_split_ptr = prepared_split.get();

        if (std::any_of(predicates_set.cbegin(), predicates_set.cend(), [this, prepared_split_ptr](const Expression* other) { return prepared_split_ptr != other and predicates->check_equivalence(*prepared_split_ptr, *other); })) {
            predicates_set.erase(prepared_split.get());
            continue;
        }

        globalPredicates.push_back(std::move(prepared_split));

    }
}

/**********************************************************************************************************************/

void SpuriousnessResult::update_stats(PLAJA::StatsBase& stats) {
    switch (spuriousnessCause) {
        case None: { break; }
        case Concretization: {
            stats.inc_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_CONCRETIZATION);
            break;
        }
#ifdef ENABLE_TERMINAL_STATE_SUPPORT
            case Terminal: {
                stats.inc_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_TERMINAL);
                break;
            }
#endif
        case GuardApp: {
            stats.inc_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_GUARD_APP);
            break;
        }
        case ActionSel: {
            stats.inc_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_ACTION_SEL);
            if (is_spurious()) { stats.inc_attr_unsigned(PLAJA::StatsUnsigned::PREDICATES_FOR_ACTION_SEL_REF, globalPredicatesAdded + localPredicatesAdded); }
            break;
        }
        case Target: {
            stats.inc_attr_unsigned(PLAJA::StatsUnsigned::REF_DUE_TO_TARGET);
            break;
        }
        default: { PLAJA_ABORT }
    }

    if (is_spurious()) {

        stats.set_attr_int(PLAJA::StatsInt::SPURIOUS_PREFIX_LENGTH, PLAJA_UTILS::cast_numeric<int>(get_spurious_prefix_length()));

        PLAJA_ASSERT(globalPredicatesAdded + localPredicatesAdded > 0)
        stats.inc_attr_unsigned(PLAJA::StatsUnsigned::GLOBAL_PREDICATES_ADDED, globalPredicatesAdded);
        if constexpr (PLAJA_GLOBAL::lazyPA) { stats.inc_attr_unsigned(PLAJA::StatsUnsigned::LOCAL_PREDICATES_ADDED, localPredicatesAdded); }

    }
}

/** Compute wp .*******************************************************************************************************/

void SpuriousnessResult::compute_wp() {

    PLAJA_ASSERT(pathWpIn->get_path_size() == get_spurious_prefix_length() + 1)

    // std::unordered_map<const Expression*, StepIndex_type, EXPRESSION_SET::ExpressionHash, EXPRESSION_SET::ExpressionEqual> firstOccurrence;

    // for (auto& pred: pathWpIn->wp_per_step(targetStep)) { firstOccurrence[pred.get()] = targetStep; }

    for (auto path_it = path->init_prefix_iterator_backwards(get_spurious_prefix_length()); !path_it.end(); ++path_it) {

        // load substitution
        VariableSubstitution var_sub(pathWpIn->get_substitution(path_it.get_step()));
        const auto& sub_non_det = pathWpIn->get_sub_non_det(path_it.get_step());

        for (const auto& pred: pathWpIn->wp_per_step(path_it.get_successor_step())) {
            auto pred_sub = var_sub.substitute(*pred, sub_non_det);

            if (pred_sub) {

                // firstOccurrence[pred_sub.get()] = path_it._step();

                if (predicates->do_compute_wp()) { // if "compute_wp = false" some predicates need single step wp computation anyways

                    pathWpIn->add_predicate(std::move(pred_sub), path_it.get_step());
                    if (predicates->do_re_sub()) { pathWpIn->add_predicate(pred->deepCopy_Exp(), path_it.get_step()); }

                } else { add_wp_out(std::move(pred_sub), path_it.get_step()); }

            } else {

                if (predicates->do_compute_wp()) { pathWpIn->add_predicate(pred->deepCopy_Exp(), path_it.get_step()); } // no substitution occurred
                else { add_wp_out(pred->deepCopy_Exp(), path_it.get_step()); } // still need to propagate to predecessor step/state

            }
        }

    }

    // move IN to OUT
    const auto path_size = pathWpIn->get_path_size();
    for (StepIndex_type step = 0; step < path_size; ++step) {

        for (auto& pred: pathWpIn->wp_per_step(step)) { add_wp_out(std::move(pred), step); }

        pathWpIn->clear(step);

    }

}

void SpuriousnessResult::compute_wp_internals() {

    for (auto path_it = path->init_prefix_iterator(get_spurious_prefix_length()); !path_it.end(); ++path_it) {

        // also wp of predicate values (externally added if excluded under entailment)
        if (do_add_predicates_internally()) {
            for (auto pred_it = predicates->init_predicate_it(); !pred_it.end(); ++pred_it) {
                add_refinement_predicate(pred_it->deepCopy_Exp(), path_it.get_step());
            }
        }

        // wp of guards (externally added if excluded under entailment)
        if (do_add_guards_internally()) {
            auto& action_op = path_it.retrieve_op();
            for (auto guard_it = action_op.guardIterator(); !guard_it.end(); ++guard_it) {
                add_refinement_predicate(guard_it->deepCopy_Exp(), path_it.get_step());
            }
        }

    }

    if (do_add_predicates_internally()) {
        for (auto pred_it = predicates->init_predicate_it(); !pred_it.end(); ++pred_it) {
            add_refinement_predicate(pred_it->deepCopy_Exp(), targetStep);
        }
    }

    compute_wp();

}

void SpuriousnessResult::add_wp_out(std::unique_ptr<Expression>&& pred, StepIndex_type step) {

    auto& predicates_set = get_new_predicates(step);

    auto prepared_splits = predicates->prepare_predicate(std::move(pred), get_used_predicates(step), get_pa_state_if_local(step));

    for (auto& prepared_split: prepared_splits) {

        auto rlt = predicates_set.insert(prepared_split.get()); // local cache

        if (not rlt.second) { continue; }

        const auto* prepared_split_ptr = prepared_split.get();

        if (std::any_of(predicates_set.cbegin(), predicates_set.cend(), [this, prepared_split_ptr](const Expression* other) { return prepared_split_ptr != other and predicates->check_equivalence(*prepared_split_ptr, *other); })) {
            predicates_set.erase(prepared_split.get());
            continue;
        }

        pathWpOut->add_predicate(std::move(prepared_split), step);

    }

}

bool SpuriousnessResult::refine(PredicatesStructure& predicates_) {
    PLAJA_ASSERT(&predicates_ == predicates) // only as parameter to restrict modifiability
    compute_wp_internals();
    STMT_IF_DEBUG(const auto cached_num_predicates = predicates->size();) // store #predicates at the beginning of refinement

    // (possibly) local predicates:

    for (StepIndex_type step = 0; step < pathWpOut->get_path_size(); ++step) {
        auto& prepared_preds = pathWpOut->wp_per_step(step);

        if (PLAJA_GLOBAL::lazyPA and do_local_refinement()) { localPredicatesAdded += prepared_preds.size(); }
        else { globalPredicatesAdded += prepared_preds.size(); }

        predicates_.add_prepared_predicates(std::move(prepared_preds), get_pa_state_if_local(step));

        pathWpOut->clear(step);
    }

    // global predicates:

    globalPredicatesAdded += globalPredicates.size();
    predicates_.add_prepared_predicates(std::move(globalPredicates), nullptr);

    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or predicates->size() >= cached_num_predicates)
    PLAJA_ASSERT(PLAJA_GLOBAL::lazyPA or globalPredicatesAdded == predicates->size() - cached_num_predicates)

    return (globalPredicatesAdded + localPredicatesAdded) > 0; // predicate set was indeed refined
}

bool SpuriousnessResult::has_new_predicates() const { return not globalPredicatesSet.first.empty() or std::any_of(predicateSet.cbegin(), predicateSet.cend(), [](const std::pair<ExpressionSet, ExpressionSet>& tmp) { return not tmp.first.empty(); }); }

/**********************************************************************************************************************/

void SpuriousnessResult::dump_concretization(const std::string& filename) const {
    PLAJA_ASSERT(not is_spurious())
    pathInstance->dump_readable(filename);
}