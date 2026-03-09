//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <sstream>
#include "solution_checker_instance_pa.h"
#include "../../parser/ast/expression/non_standard/problem_instance_expression.h"
#include "../../parser/ast/expression/array_value_expression.h"
#include "../../globals.h"
#include "../information/model_information.h"
#include "../smt/base/solution_check_wrapper.h"
#include "../states/state_values.h"
#include "../successor_generation/action_op.h"
#include "pa_states/abstract_path.h"
#include "pa_states/abstract_state.h"
#include "pa_transition_structure.h"
#include "solution_checker_pa.h"

SolutionCheckerInstancePa::SolutionCheckerInstancePa(const SolutionCheckerPa& solution_checker, const AbstractState* pa_src, ActionLabel_type action_label, ActionOpID_type action_op, UpdateIndex_type update_index, const AbstractState* pa_target):
    SolutionCheckerInstance(solution_checker, pa_target ? 2 : 1)
    , paSrc(pa_src)
    , actionLabel(action_label)
    , actionOp(action_op)
    , updateIndex(update_index)
    , paTarget(pa_target) {
    PLAJA_ASSERT(paSrc)
}

SolutionCheckerInstancePa::SolutionCheckerInstancePa(const SolutionCheckerPa& solution_checker, const PATransitionStructure& transition_structure):
    SolutionCheckerInstance(solution_checker, transition_structure._target() ? 2 : 1)
    , paSrc(transition_structure._source())
    , actionLabel(transition_structure._action_label())
    , actionOp(transition_structure._action_op_id())
    , updateIndex(transition_structure._update_index())
    , paTarget(transition_structure._target()) {
    PLAJA_ASSERT(paSrc)
}

SolutionCheckerInstancePa::~SolutionCheckerInstancePa() = default;

const SolutionCheckerPa& SolutionCheckerInstancePa::get_checker_pa() const { return PLAJA_UTILS::cast_ref<SolutionCheckerPa>(get_checker()); }

bool SolutionCheckerInstancePa::check() const {
    return get_checker_pa().check(*get_solution(0), paSrc, actionLabel, actionOp, updateIndex, paTarget, paTarget ? get_solution(1) : nullptr);
}

bool SolutionCheckerInstancePa::check(const SolutionCheckWrapper& wrapper) {
    set(wrapper);
    return get_checker_pa().check(*get_solution(0), paSrc, actionLabel, actionOp, updateIndex, paTarget, paTarget ? get_solution(1) : nullptr);
}

void SolutionCheckerInstancePa::set(const SolutionCheckWrapper& wrapper) {

    set_solution(wrapper.to_state(0, false), 0);

    if (paTarget) {
        PLAJA_ASSERT(get_solution_size() == 2)
        PLAJA_ASSERT(actionOp != ACTION::noneOp and updateIndex != ACTION::noneUpdate)
        set_successor(wrapper, 0, get_checker_pa().get_action_op(actionOp).get_update(updateIndex));
    }

}

PLAJA::floating SolutionCheckerInstancePa::policy_selection_delta() const { return get_checker_pa().policy_selection_delta(); }

bool SolutionCheckerInstancePa::policy_chosen_with_tolerance() const { return get_checker_pa().policy_chosen_with_tolerance(); }

StateValues SolutionCheckerInstancePa::get_solution_constructor() const {
    // return paSrc ? get_checker_pa().get_solution_constructor(*paSrc) : get_checker_pa().get_solution_constructor();
    PLAJA_ASSERT(paSrc)
    return get_checker_pa().get_solution_constructor(*paSrc);
}

void SolutionCheckerInstancePa::dump(const std::string& filename) const {
    std::unique_ptr<ProblemInstanceExpression> problem_instance(new ProblemInstanceExpression());

    problem_instance->set_target_step(paTarget ? 1 : 0);
    problem_instance->set_policy_target_step(get_checker_pa().has_policy() ? 1 : 0);
    problem_instance->add_op_path_step(actionOp, updateIndex);
    if (paSrc) {
        problem_instance->set_predicates(paSrc->get_predicates());
        problem_instance->add_pa_state_path_step(PLAJA_UTILS::cast_unique<ArrayValueExpression>(paSrc->to_array_value()));
        if (paTarget) { problem_instance->add_pa_state_path_step(PLAJA_UTILS::cast_unique<ArrayValueExpression>(paTarget->to_array_value())); }
    } else { PLAJA_ASSERT(not paTarget) }

    PropertyExpression::write_to_properties(std::move(problem_instance), filename);
}

void SolutionCheckerInstancePa::dump_readable(const std::string& filename) const {
    const auto* model = &get_checker_pa().get_model();

    std::stringstream ss;
    ss << "PA-Problem:" << std::endl;
    if (paSrc) { ss << paSrc->to_array_value(model)->to_string(true) << std::endl; }
    if (0 < get_solution_size() and get_solution(0)) { ss << get_solution(0)->to_str(model) << std::endl; }

    if (actionOp == ACTION::noneOp) {
        if (actionLabel != ACTION::noneLabel) { ss << ActionOp::label_to_str(actionLabel) << std::endl; }
    } else {
        ss << get_checker_pa().get_action_op(actionOp).to_str(model, true) << std::endl;
    }

    if (paTarget) { ss << paTarget->to_array_value(model)->to_string(true) << std::endl; }
    if (1 < get_solution_size() and get_solution(1)) { ss << get_solution(1)->to_str(model) << std::endl; }

    PLAJA_UTILS::write_to_file(filename, ss.str(), true);
}

/**********************************************************************************************************************/
/**********************************************************************************************************************/

SolutionCheckerInstancePaPath::SolutionCheckerInstancePaPath(const SolutionCheckerPa& solution_checker, const AbstractPath& path, StepIndex_type target_step, StepIndex_type policy_target_step, bool pa_state_aware, const Expression* start, bool include_init, const Expression* reach):
    SolutionCheckerInstance(solution_checker, target_step + 1)
    , path(&path)
    , targetStep(target_step)
    , policyTargetStep(policy_target_step)
    , start(start)
    , reach(reach)
    , paStateAware(pa_state_aware)
    , includeInit(include_init)
    , lastDelta(0) {
    PLAJA_ASSERT(policy_target_step <= target_step)
}

SolutionCheckerInstancePaPath::~SolutionCheckerInstancePaPath() = default;

const SolutionCheckerPa& SolutionCheckerInstancePaPath::get_checker_pa() const { return PLAJA_UTILS::cast_ref<SolutionCheckerPa>(get_checker()); }

std::unique_ptr<SolutionCheckerInstancePaPath> SolutionCheckerInstancePaPath::copy_problem() const {
    return std::make_unique<SolutionCheckerInstancePaPath>(get_checker_pa(), *path, targetStep, policyTargetStep, paStateAware, start, includeInit, reach);
}

/**********************************************************************************************************************/

/* Auxiliary class to reduce code duplication between const and non-const check. */
struct SolutionCheckerInstancePaPathAux {

    template<typename SolutionCheckerInstancePaPath_type>
    static bool check(SolutionCheckerInstancePaPath_type& instance, const SolutionCheckWrapper* wrapper) {

        PLAJA_ASSERT(bool(std::is_same_v<SolutionCheckerInstancePaPath_type, SolutionCheckerInstancePaPath>) or bool(std::is_same_v<SolutionCheckerInstancePaPath_type, const SolutionCheckerInstancePaPath>))
        PLAJA_STATIC_ASSERT(not bool(std::is_same_v<SolutionCheckerInstancePaPath, const SolutionCheckerInstancePaPath>))
        constexpr bool update_solution = std::is_same_v<SolutionCheckerInstancePaPath_type, SolutionCheckerInstancePaPath>;
        PLAJA_ASSERT(not update_solution or wrapper)

        instance.lastDelta = 0; // Reset.

        PLAJA::floating max_delta = 0;

        const auto& checker = instance.get_checker_pa();

        if constexpr (update_solution) { instance.set_solution(wrapper->to_state(0, false), 0); }
        const auto* current = instance.get_solution(0);
        PLAJA_ASSERT(checker.check_state(*current))

        /* Possible start constraints. */
        if (instance.includeInit and instance.start) {
            if (checker.get_model_info().get_initial_values() != (*current) and not instance.start->evaluate_integer(*current)) {
                return false;
            }
        } else if (instance.includeInit and checker.get_model_info().get_initial_values() != (*current)) { return false; }
        else if (instance.start and not instance.start->evaluate_integer(*current)) { return false; }

        auto it = instance.path->init_prefix_iterator(instance.targetStep);
        for (; !it.end(); ++it) {

            const auto& action_op = checker.get_action_op(it.get_op_id());

            /* Abstract state. */
            if (instance.paStateAware and not checker.check_abstraction(*current, it.get_pa_state())) { return false; }

            /* Check terminal. */
            PLAJA_ASSERT_EXPECT(not checker.check_terminal(*current))
            if (PLAJA_GLOBAL::enableTerminalStateSupport and checker.check_terminal(*current)) { return false; }

            /* Check guard. */
            if (not action_op.is_enabled(*current)) { return false; } // Directly call action op since we include header anyway, in contrast to abstract state ...

            /* Check policy. */
            if (it.get_step() < instance.policyTargetStep and checker.has_policy()) {
                if (checker.check_policy(*current, action_op._action_label(), nullptr)) { max_delta = std::max(max_delta, instance.policy_selection_delta()); }
                else { return false; }
            }

            // get successor
            if constexpr (update_solution) { instance.set_successor(*wrapper, it.get_step(), action_op.get_update(it.get_update_index())); }
            STMT_IF_DEBUG(PLAJA_GLOBAL::push_additional_outputs_frame(true))
            PLAJA_ASSERT(checker.agrees(*current, action_op, it.get_update_index(), *instance.get_solution(it.get_successor_step())))
            STMT_IF_DEBUG(PLAJA_GLOBAL::pop_additional_outputs_frame())
            current = instance.get_solution(it.get_successor_step());
            PLAJA_ASSERT(checker.check_state(*current)) // TODO: For now keep as assertion. Though, presumably not guaranteed (set_successor() in solution_checker_instance.cpp)

        }

        if (instance.targetStep < instance.path->get_target_step()) {

            if (instance.paStateAware and not checker.check_abstraction(*current, it.get_pa_state())) { return false; }

            if (PLAJA_GLOBAL::enableTerminalStateSupport and checker.check_terminal(*current)) { return false; }

        } else {

            /* Target abstract state. */
            if (instance.paStateAware and not checker.check_abstraction(*current, instance.path->get_pa_target_state())) { return false; }

            /* Check terminal. */
            if (PLAJA_GLOBAL::enableTerminalStateSupport and not PLAJA_GLOBAL::reachMayBeTerminal and checker.check_terminal(*current)) { return false; }
            if constexpr (PLAJA_GLOBAL::reachMayBeTerminal) { PLAJA_ASSERT_EXPECT(not checker.check_terminal(*current)) }

            /* Reach condition. */
            if (instance.reach and not instance.reach->evaluate_integer(*current)) { return false; }

        }

        instance.lastDelta = max_delta;
        return true;
    }

};

/**********************************************************************************************************************/

bool SolutionCheckerInstancePaPath::check() const {
    return SolutionCheckerInstancePaPathAux::check<const SolutionCheckerInstancePaPath>(*this, nullptr);
};

bool SolutionCheckerInstancePaPath::check(const SolutionCheckWrapper& wrapper) {
    return SolutionCheckerInstancePaPathAux::check<SolutionCheckerInstancePaPath>(*this, &wrapper);
}

void SolutionCheckerInstancePaPath::set(const SolutionCheckWrapper& wrapper) {
    set_solution(wrapper.to_state(0, false), 0); // Locations via constructor.

    const auto& checker = get_checker_pa();

    for (auto it = path->init_prefix_iterator(targetStep); !it.end(); ++it) {

        const auto& action_op = checker.get_action_op(it.get_op_id());

        set_successor(wrapper, it.get_step(), action_op.get_update(it.get_update_index()));

    }
}

PLAJA::floating SolutionCheckerInstancePaPath::policy_selection_delta() const { return lastDelta; }

bool SolutionCheckerInstancePaPath::policy_chosen_with_tolerance() const { return lastDelta > 0; }

StateValues SolutionCheckerInstancePaPath::get_solution_constructor() const {
    return get_checker_pa().get_solution_constructor(path->get_pa_source_state());
    // return paStateAware ? get_checker_pa().get_solution_constructor(path->get_pa_source_state()) : get_checker_pa().get_solution_constructor();
}

/**********************************************************************************************************************/

const StateValues& SolutionCheckerInstancePaPath::get_target() const {
    PLAJA_ASSERT(get_solution(targetStep))
    return *get_solution(targetStep);
}

std::unique_ptr<StateValues> SolutionCheckerInstancePaPath::retrieve_target() { return retrieve_solution(targetStep); }

const StateValues& SolutionCheckerInstancePaPath::get_policy_target() const {
    PLAJA_ASSERT(get_solution(policyTargetStep))
    return *get_solution(policyTargetStep);
}

std::unique_ptr<StateValues> SolutionCheckerInstancePaPath::retrieve_policy_target() { return retrieve_solution(policyTargetStep); }

void SolutionCheckerInstancePaPath::dump(const std::string& filename) const {
    std::unique_ptr<ProblemInstanceExpression> problem_instance(new ProblemInstanceExpression());

    problem_instance->set_target_step(targetStep);
    problem_instance->set_policy_target_step(policyTargetStep);
    problem_instance->set_start(start);
    problem_instance->set_reach(reach);
    problem_instance->set_includes_init(includeInit);

    /* Op path. */
    problem_instance->reserve_op_path();
    for (StepIndex_type step = 0; step < targetStep; ++step) {
        problem_instance->add_op_path_step(path->get_op_id(step), path->get_update_index(step));
    }

    /* Pa state path. */
    if (paStateAware) {

        problem_instance->set_predicates(&path->get_predicates());

        for (StepIndex_type step = 0; step < targetStep; ++step) {
            problem_instance->add_pa_state_path_step(PLAJA_UTILS::cast_unique<ArrayValueExpression>(path->get_pa_state(step).to_array_value()));
        }

    }

    PropertyExpression::write_to_properties(std::move(problem_instance), filename);
}

void SolutionCheckerInstancePaPath::dump_readable(const std::string& filename) const {
    const auto* model = &get_checker_pa().get_model();

    std::stringstream ss;
    ss << "PA-Path:" << std::endl;
    ss << "Target step: " << targetStep << std::endl;
    ss << "Policy target step: " << targetStep << std::endl;
    if (start) { ss << "Has start." << std::endl; }
    if (includeInit) { ss << "Includes init." << std::endl; }
    if (reach) { ss << "Has reach." << std::endl; }

    for (auto it = path->init_path_iterator(); !it.end(); ++it) {
        if (paStateAware) { ss << it.get_pa_state().to_array_value(model)->to_string(true) << std::endl; }
        if (it.get_step() < get_solution_size() and get_solution(it.get_step())) { ss << get_solution(it.get_step())->to_str(model) << std::endl; }
        const auto& op = it.retrieve_op();
        ss << op.to_str(model, true) << std::endl;
        ss << ActionOp::update_to_str(it.get_update_index()) << std::endl;
    }

    if (paStateAware) { ss << path->get_pa_target_state().to_array_value(model)->to_string(true) << std::endl; }
    if (path->get_target_step() < get_solution_size() and get_solution(path->get_target_step())) { ss << get_solution(path->get_target_step())->to_str(model) << std::endl; }

    PLAJA_UTILS::write_to_file(filename, ss.str(), true);

}

/// Extracts StateValues corresponding to a concrete unsafe path excluding unsafe states
std::unordered_set<std::unique_ptr<StateBase>> SolutionCheckerInstancePaPath::get_states_along_path() const{
    std::unordered_set<std::unique_ptr<StateBase>> states;
    std::cout << "unsafe path size: " << get_solution_size() << '\n';
    auto prefix_length = get_solution_size() - 1;
    for (auto it = path->init_prefix_iterator(prefix_length); !it.end(); ++it) {
        const auto s = get_solution(it.get_step());
        std::cout << "solution state: " << s->to_str() << '\n';
        states.emplace(s->to_ptr());
    }
    return states;
}

/// Extracts StateValues corresponding to a concrete unsafe path
std::vector<StateValues> SolutionCheckerInstancePaPath::get_concrete_unsafe_path() const {
    std::cout << "Getting concrete unsafe path from: ";
    path->dump();
    std::vector<StateValues> states;
    std::cout << "unsafe path size: " << get_solution_size() << '\n';
    auto prefix_length = get_solution_size();
    for (auto it = path->init_prefix_iterator(prefix_length); !it.end(); ++it) {
        const auto s = get_solution(it.get_step());
        std::cout << "solution state: " << s->to_str() << '\n';
        states.push_back(*s);
    }
    return states;
}