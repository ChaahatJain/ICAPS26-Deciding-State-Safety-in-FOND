//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <cmath>
#include "selection_refinement.h"
#include "../../../exception/constructor_exception.h"
#include "../../../option_parser/option_parser_aux.h"
#include "../../../parser/ast/expression/binary_op_expression.h"
#include "../../../parser/ast/expression/constant_value_expression.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/type/declaration_type.h"
#include "../../../stats/stats_base.h"
#include "../../../utils/floating_utils.h"
#include "../../../utils/rng.h"
#include "../../factories/configuration.h"
#include "../../information/model_information.h"
#include "../../non_prob_search/policy/policy.h"
#include "../../states/state_values.h"
#include "../pa_states/pa_state_base.h"
#include "spuriousness_result.h"
#include "variable_selection.h"

// forward declaration

namespace PLAJA_OPTION::NN_SAT_CHECKER_PA { extern bool is_relaxed_nn_sat_mode(const PLAJA::Configuration& config); }

/**********************************************************************************************************************/

namespace PLAJA_OPTION_DEFAULTS {

    const std::string selection_refinement("all"); // NOLINT(cert-err58-cpp)
    const std::string split_point("bs"); // NOLINT(cert-err58-cpp)
    const std::string entailment_mode("entailment-only"); // NOLINT(cert-err58-cpp)

}

namespace PLAJA_OPTION {

    const std::string selection_refinement("selection-refinement"); // NOLINT(cert-err58-cpp)
    const std::string split_point("split-point"); // NOLINT(cert-err58-cpp)
    const std::string entailment_mode("entailment-mode"); // NOLINT(cert-err58-cpp)

    namespace SELECTION_REFINEMENT {

        const std::unordered_map<std::string, SelectionRefinement::Mode> stringToSelectionRefinement { // NOLINT(cert-err58-cpp)
            { "concretization-exclusion", SelectionRefinement::ConcretizationExclusion },
            { "ws-all",                   SelectionRefinement::WsAll },
            { "ws-max-abs",               SelectionRefinement::WsMaxAbs },
            { "ws-max-rel",               SelectionRefinement::WsMaxRel },
            { "ws-min-abs",               SelectionRefinement::WsMinAbs },
            { "ws-min-rel",               SelectionRefinement::WsMinRel },
            { "ws-var-sel",               SelectionRefinement::WsVarSel },
            { "ws-sensitive",             SelectionRefinement::WsSensitiveOne },
            { "ws-search-smt-c",          SelectionRefinement::WsSmtSearchC },
            { "ws-search-smt-pa",         SelectionRefinement::WsSmtSearchPa },
            { "ws-search-smt",            SelectionRefinement::WsSmtSearch },
            { "ws-random-one",            SelectionRefinement::WsRandomOne },
            { "ws-random",                SelectionRefinement::WsRandom },
            { "all",                      SelectionRefinement::All },
            { "ws-interval",              SelectionRefinement::WsInterval},
            { "interval-exclusion",       SelectionRefinement::IntervalExclusion},
        };

        const std::unordered_map<std::string, SelectionRefinement::SplitPoint> stringToSplitPosition { // NOLINT(cert-err58-cpp)
            { "near-concretization",   SelectionRefinement::NearConcretization },
            { "near-witness",          SelectionRefinement::NearWitness },
            { "middle",                SelectionRefinement::Middle },
            { "random",                SelectionRefinement::Random },
            { "ws-search",             SelectionRefinement::WsSearch },
            { "var-sel-eps",           SelectionRefinement::VarSelEps },
            { "global-split-ws",       SelectionRefinement::GlobalSplitWs },
            { "global-split-pa-state", SelectionRefinement::GlobalSplitPaState },
            { "bs",                    SelectionRefinement::BinarySplit },
        };

        const std::unordered_map<std::string, SelectionRefinement::EntailmentMode> stringToEntailmentMode { // NOLINT(cert-err58-cpp)
            { "none",            SelectionRefinement::EntailmentMode::None },
            { "use-entailment",  SelectionRefinement::EntailmentMode::UseEntailment },
            { "entailment-only", SelectionRefinement::EntailmentMode::EntailmentOnly },
        };

        void add_options(PLAJA::OptionParser& option_parser) {
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::selection_refinement, PLAJA_OPTION_DEFAULTS::selection_refinement);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::split_point, PLAJA_OPTION_DEFAULTS::split_point);
            OPTION_PARSER::add_option(option_parser, PLAJA_OPTION::entailment_mode, PLAJA_OPTION_DEFAULTS::entailment_mode);
            PLAJA_OPTION::VARIABLE_SELECTION::add_options(option_parser);
        }

        void print_options() {
            OPTION_PARSER::print_option(PLAJA_OPTION::selection_refinement, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULTS::selection_refinement, "How to refine selection related spuriousness.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(stringToSelectionRefinement), false);
            OPTION_PARSER::print_option(PLAJA_OPTION::split_point, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULTS::split_point, "Split point for witness-based splitting.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(stringToSplitPosition), false);
            OPTION_PARSER::print_option(PLAJA_OPTION::entailment_mode, OPTION_PARSER::valueStr, PLAJA_OPTION_DEFAULTS::entailment_mode, "How to proceed for entailed state-variable-values during witness splitting.", true);
            OPTION_PARSER::print_additional_specification(PLAJA::OptionParser::print_supported_options(PLAJA_OPTION::SELECTION_REFINEMENT::stringToEntailmentMode), false);
            PLAJA_OPTION::VARIABLE_SELECTION::print_options();
        }

    }

}

/**********************************************************************************************************************/

namespace SELECTION_REFINEMENT {

    struct SplitVarsSelection {

        template<bool relative>
        inline static double diff(VariableIndex_type state_index, const StateBase& witness, const StateBase& concretization, const ModelInformation& model_info) {
            const double diff = std::abs(concretization[state_index] - witness[state_index]);
            PLAJA_ASSERT(PLAJA_FLOATS::lt(model_info.get_lower_bound(state_index), model_info.get_upper_bound(state_index), PLAJA::floatingPrecision))
            if constexpr (relative) { return diff / (model_info.get_upper_bound(state_index) - model_info.get_lower_bound(state_index)); }
            else { return diff; }
        }

        template<bool max, bool relative>
        inline static void witness_split_priority(SpuriousnessResult& result, const SelectionRefinement& parent, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) {
            auto candidates = parent.witness_split_candidates(witness, concretization);
            PLAJA_ASSERT(not candidates.empty())

            const auto& model_info = parent.policyInterface->get_model_info();
            auto it = candidates.cbegin();
            const auto end = candidates.cend();
            //
            auto* candidate = *it;
            auto priority = diff<relative>(candidate->get_variable_index(), witness, concretization, model_info);
            //
            for (++it; it != end; ++it) {
                auto priority_n = diff<relative>((*it)->get_variable_index(), witness, concretization, model_info);
                if constexpr (max) {
                    if (priority_n > priority) {
                        candidate = *it;
                        priority = priority_n;
                    }
                } else {
                    if (priority_n < priority) {
                        candidate = *it;
                        priority = priority_n;
                    }
                }
            }

            return parent.add_refinement_preds(result, { candidate }, action, witness, concretization);
        }

    };

    template<bool witness_lt>
    struct SplitBinarySearch {

    private:
        const Policy* policy;
        const LValueExpression* var;
        const VariableIndex_type stateIndex; // NOLINT(*-avoid-const-or-ref-data-members)
        const bool isFloating; // NOLINT(*-avoid-const-or-ref-data-members)
        const ActionLabel_type action; // NOLINT(*-avoid-const-or-ref-data-members)
        StateValues tmpState;
        const PLAJA::floating concreteValue; // NOLINT(*-avoid-const-or-ref-data-members)
        const PLAJA::floating splitInterval; // NOLINT(*-avoid-const-or-ref-data-members)

        static const unsigned relativeSplitPrecision = 100;

        /*
         * Invariant: witness[stateIndex] = witness_lt ? lb : ub and policy(witness) = action.
         */
        std::unique_ptr<Expression> bs_search_split(PLAJA::floating lb, PLAJA::floating ub) { // NOLINT(*-no-recursion)
            PLAJA_ASSERT(isFloating or (PLAJA_FLOATS::is_integer(lb) and PLAJA_FLOATS::is_integer(ub)))
            PLAJA_ASSERT(PLAJA_FLOATS::equal(witness_lt ? lb : ub, tmpState[stateIndex]))
            PLAJA_ASSERT(policy->is_chosen(tmpState, action))

            if (PLAJA_FLOATS::equal(lb, ub, splitInterval)) { return tmpState.get_value(*var); }

            auto candidate = (ub + lb) / 2;
            if (not isFloating) { candidate = witness_lt ? std::floor(candidate) : std::ceil(candidate); }
            PLAJA_ASSERT(isFloating or PLAJA_FLOATS::is_integer(candidate))
            tmpState.assign<false>(stateIndex, candidate);
            const bool is_selected = policy->is_chosen(tmpState, action);

            if (is_selected) {

                if (witness_lt) { return bs_search_split(candidate, ub); }
                else { return bs_search_split(lb, candidate); }

            } else {

                if (witness_lt) {

                    tmpState.assign<false>(stateIndex, lb);
                    return bs_search_split(lb, candidate);

                } else {

                    tmpState.assign<false>(stateIndex, ub);
                    return bs_search_split(candidate, ub);

                }

            }

            PLAJA_ABORT

        }

    public:
        SplitBinarySearch(const Policy& policy, const LValueExpression& var, ActionLabel_type action, const StateBase& witness, PLAJA::floating concrete_value): // NOLINT(*-easily-swappable-parameters)
            policy(&policy)
            , var(&var)
            , stateIndex(var.get_variable_index())
            , isFloating(var.is_floating())
            , action(action)
            , tmpState(witness.to_state_values())
            , concreteValue(concrete_value)
            , splitInterval(isFloating ? std::abs(witness[stateIndex] - concrete_value) / relativeSplitPrecision : 1) {
            PLAJA_ASSERT(PLAJA_FLOATS::gte(splitInterval, PREDICATE::predicateSplitPrecision))
            PLAJA_ASSERT(witness_lt ? PLAJA_FLOATS::lt(witness[stateIndex], concrete_value, PLAJA::floatingPrecision) : PLAJA_FLOATS::gt(witness[stateIndex], concrete_value, PLAJA::floatingPrecision))
        }

        std::unique_ptr<Expression> search_split() { return witness_lt ? bs_search_split(tmpState[stateIndex], concreteValue) : bs_search_split(concreteValue, tmpState[stateIndex]); }

    };
}

/**********************************************************************************************************************/

SelectionRefinement::SelectionRefinement(const PLAJA::Configuration& config, const Jani2Interface& policy_interface):
    policyInterface(&policy_interface)
    , policy(&policyInterface->load_policy(config))
    , mode(config.get_option(PLAJA_OPTION::SELECTION_REFINEMENT::stringToSelectionRefinement, PLAJA_OPTION::selection_refinement))
    , splitPoint(config.get_option(PLAJA_OPTION::SELECTION_REFINEMENT::stringToSplitPosition, PLAJA_OPTION::split_point))
    , entailmentMode(config.get_option(PLAJA_OPTION::SELECTION_REFINEMENT::stringToEntailmentMode, PLAJA_OPTION::entailment_mode))
    , varSelection()
    , rng(has_random_mode() or has_random_split_point() ? PLAJA_GLOBAL::rng.get() : nullptr)
    , stats(config.get_sharable_ptr<PLAJA::StatsBase>(PLAJA::SharableKey::STATS))
    , accumulatedDistanceWsStates(0)
    , wsStatsCounter(0) {

    if (std::unordered_set<SelectionRefinement::Mode> { Mode::WsVarSel, Mode::WsSensitiveOne, Mode::WsSmtSearchC, Mode::WsSmtSearchPa, Mode::WsSmtSearch }.count(mode)) { varSelection = std::make_unique<VariableSelection>(config); }

    if (std::unordered_set<SelectionRefinement::Mode> { Mode::WsSmtSearchC, Mode::WsSmtSearchPa, Mode::WsSmtSearch }.count(mode)) {
        PLAJA_LOG_IF(splitPoint != SplitPoint::VarSelEps, PLAJA_UTILS::string_f("Warning: Split point is fixed for ws-search-smt mode(s)."))
        splitPoint = SplitPoint::VarSelEps;
    }

    if (splitPoint == SelectionRefinement::VarSelEps and not std::unordered_set<SelectionRefinement::Mode> { Mode::WsVarSel, Mode::WsSmtSearchC, Mode::WsSmtSearchPa, Mode::WsSmtSearch }.count(mode)) { throw ConstructorException(__PRETTY_FUNCTION__, "var-sel-eps split only for var-sel or ws-search-smt modes."); }

    if (std::unordered_set<SelectionRefinement::Mode> { Mode::WsSmtSearchC, Mode::WsSmtSearchPa, Mode::WsSmtSearch }.count(mode)) {
        if (entailmentMode == UseEntailment) { throw ConstructorException(__PRETTY_FUNCTION__, "When using ws-search-smt use-entailment mode is not supported."); }
    }

    if (stats) {
        stats->add_unsigned_stats(PLAJA::StatsUnsigned::EntailedSplits, "EntailedSplits", 0);
        // stats->add_unsigned_stats(PLAJA::StatsUnsigned::RefWithEntailment, "RefWithEntailment", 0);
    }

    if (needs_witness() and stats) {
        stats->add_unsigned_stats(PLAJA::StatsUnsigned::WsFallBack, "WsFallBack", 0);
        stats->add_unsigned_stats(PLAJA::StatsUnsigned::WsCandidates, "WsCandidates", 0);
        stats->add_unsigned_stats(PLAJA::StatsUnsigned::WsVars, "WsVars", 0);
        stats->add_double_stats(PLAJA::StatsDouble::AvDistanceWsState, "AvSDistanceWsState", 0);
        stats->add_double_stats(PLAJA::StatsDouble::AvDistanceWsStateCandidates, "AvSDistanceWsStateCandidates", 0);
        // stats->add_double_stats(PLAJA::StatsDouble::AvDistanceWs, "AvSDistanceWs", 0);
    }

}

SelectionRefinement::~SelectionRefinement() = default;

bool SelectionRefinement::needs_witness() const {
    return mode != SelectionRefinement::ConcretizationExclusion and mode != SelectionRefinement::All;
}

bool SelectionRefinement::needs_concretization() const {
    return mode != SelectionRefinement::All or (splitPoint != SelectionRefinement::BinarySplit and splitPoint != SelectionRefinement::GlobalSplitPaState);
}

/**********************************************************************************************************************/

void SelectionRefinement::update_ws_stats(const StateBase& witness, const StateBase& concretization) const { // NOLINT(*-easily-swappable-parameters)

    wsStatsCounter += 1;

    if (not stats) { return; }

    for (auto it = policyInterface->get_model_info().stateIndexIteratorInt(true); !it.end(); ++it) {

        const auto state_index = it.state_index();
        const auto concretization_value = concretization.get_int(state_index);
        const auto witness_value = witness.get_int(state_index);

        if (concretization_value != witness_value) {

            stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsCandidates);

            accumulatedDistanceWsStates += std::abs(concretization_value - witness_value) / it.range(); // NOLINT(*-integer-division)

        }

    }

    for (auto it = policyInterface->get_model_info().stateIndexIteratorFloat(); !it.end(); ++it) {

        const auto state_index = it.state_index();
        const auto concretization_value = concretization.get_float(state_index);
        const auto witness_value = witness.get_float(state_index);

        if (PLAJA_FLOATS::non_equal(concretization_value, witness_value, PLAJA::floatingPrecision)) {

            stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsCandidates);

            accumulatedDistanceWsStates += std::abs(concretization_value - witness_value) / (it.upper_bound_float() - it.lower_bound_float());

        }

    }

    stats->set_attr_double(PLAJA::StatsDouble::AvDistanceWsState, accumulatedDistanceWsStates / PLAJA_UTILS::cast_numeric<double>(concretization.size() * wsStatsCounter));
    stats->set_attr_double(PLAJA::StatsDouble::AvDistanceWsStateCandidates, accumulatedDistanceWsStates / PLAJA_UTILS::cast_numeric<double>(stats->get_attr_unsigned(PLAJA::StatsUnsigned::WsCandidates)));

}

/**********************************************************************************************************************/
#ifdef USE_VERITAS
std::list<const LValueExpression*> SelectionRefinement::witness_split_candidates(const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const {
    std::list<const LValueExpression*> candidates;
    PLAJA_ASSERT(concretization.size() - 1 == box_witness.size())
    // std::cout << "Concretization: "; concretization.dump(true);
    for (auto it = policyInterface->init_input_feature_iterator(true); !it.end(); ++it) {

        const auto* var = it.get_input_feature_expression();
        PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_ptr_type<VariableExpression>(var)) // sanity
        PLAJA_ASSERT(var->get_variable_index() == it.get_input_feature_index()) // sanity
        if (mode == SelectionRefinement::IntervalExclusion) {
            candidates.push_back(var);
        } else {
            const auto state_index = it.get_input_feature_index();
            const auto interval = box_witness[state_index - 1];

            if (state_index < concretization.get_int_state_size()) {
                const auto val = concretization.get_int(state_index);
                // std::cout << "State Index: " << state_index << ", interval " << interval << "Val: " << val << std::endl;
                if (val < interval.lo or val >= interval.hi) { candidates.push_back(var); }
            } else {
                const auto val = concretization.get_float(state_index);
                if (val < interval.lo or val >= interval.hi) { candidates.push_back(var); }
            }
        }
    }

    PLAJA_ASSERT(not candidates.empty())

    return candidates;
}
#endif

std::list<const LValueExpression*> SelectionRefinement::witness_split_candidates(const StateBase& witness, const StateBase& concretization) const {
    std::list<const LValueExpression*> candidates;

    PLAJA_ASSERT(concretization != witness and concretization.size() == witness.size())

    for (auto it = policyInterface->init_input_feature_iterator(true); !it.end(); ++it) {

        const auto* var = it.get_input_feature_expression();
        PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_ptr_type<VariableExpression>(var)) // sanity
        PLAJA_ASSERT(var->get_variable_index() == it.get_input_feature_index()) // sanity
        const auto state_index = it.get_input_feature_index();

        if (concretization.non_equal(witness, state_index)) { candidates.push_back(var); }
    }

    PLAJA_ASSERT(not candidates.empty())

    return candidates;
}
#ifdef USE_VERITAS
void SelectionRefinement::add_refinement_preds(SpuriousnessResult& result, const std::list<const LValueExpression*>& split_vars, ActionLabel_type action, const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const {
    const auto entailed_vars = entailmentMode != EntailmentMode::None ? result.compute_entailments_set(concretization, false) : std::unordered_set<VariableID_type> {};

    // first add entailed splits:
    if (not entailed_vars.empty()) {
        for (const auto* var: split_vars) {
            if (entailed_vars.count(var->get_variable_index())) {
                if (mode == SelectionRefinement::IntervalExclusion) {
                    add_entailed_refinement_preds(result, *var, box_witness);
                } else {
                    add_entailed_refinement_preds(result, *var, concretization, box_witness);
                }
            }
        }
    }

    // non-entailed splits:
    if (entailmentMode == EntailmentMode::EntailmentOnly and result.has_new_predicates()) { return; }

    for (const auto* var: split_vars) {
        if (not entailed_vars.count(var->get_variable_index())) {
            if (mode == SelectionRefinement::IntervalExclusion) {
                result.add_refinement_predicate_wp(create_lower_split(*var, action, box_witness));
                result.add_refinement_predicate_wp(create_upper_split(*var, action, box_witness));
                stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsVars, 2);
            } else {
                result.add_refinement_predicate_wp(create_split(*var, action, box_witness, concretization));
                stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsVars);
            }
        }
    }
}
#endif

void SelectionRefinement::add_refinement_preds(SpuriousnessResult& result, const std::list<const LValueExpression*>& split_vars, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const {
    const auto entailed_vars = entailmentMode != EntailmentMode::None ? result.compute_entailments_set(concretization, false) : std::unordered_set<VariableID_type> {};

    // first add entailed splits:
    if (not entailed_vars.empty()) {
        for (const auto* var: split_vars) {
            if (entailed_vars.count(var->get_variable_index())) {
                add_entailed_refinement_preds(result, *var, concretization, witness);
            }
        }
    }

    // non-entailed splits:
    if (entailmentMode == EntailmentMode::EntailmentOnly and result.has_new_predicates()) { return; }

    if (std::unordered_set<SplitPoint> { BinarySplit, GlobalSplitPaState, GlobalSplitWs }.count(splitPoint)) {

        const auto& predicates = result.get_predicates();
        std::unique_ptr<std::pair<StateValues, StateValues>> state_bounds(nullptr);

        if (splitPoint == GlobalSplitPaState) {
            const auto& pa_state = result.get_pa_state(result.get_spurious_prefix_length());
            state_bounds = std::make_unique<std::pair<StateValues, StateValues>>(pa_state.compute_bounds(policyInterface->get_model_info()));
            predicates.set_interval_for_propose_split(&state_bounds->first, &state_bounds->second);
        }

        for (const auto* var: split_vars) {
            if (not entailed_vars.count(var->get_variable_index())) {

                if (splitPoint == GlobalSplitWs) {
                    if (concretization.lt(witness, var->get_variable_index())) { predicates.set_interval_for_propose_split(&concretization, &witness); }
                    else { predicates.set_interval_for_propose_split(&witness, &concretization); }
                }

                // TODO might want option to add split only locally, i.p., for GlobalSplitPaState, GlobalSplitWs
                auto split = predicates.propose_split(*var, result.get_used_predicates(result.get_spurious_prefix_length()), result.get_pa_state_if_local(result.get_spurious_prefix_length()));
                if (split) {
                    result.add_refinement_predicate(std::move(split));
                    stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsVars);
                }
            }
        }

        predicates.set_interval_for_propose_split();

    } else {

        for (const auto* var: split_vars) {
            if (not entailed_vars.count(var->get_variable_index())) {
                result.add_refinement_predicate_wp(create_split(*var, action, witness, concretization));
                stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsVars);
            }
        }
    }
}

#ifdef USE_VERITAS
std::unique_ptr<Expression> SelectionRefinement::create_lower_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness) const {
    const VariableIndex_type state_index = var.get_variable_index();
    const auto witness_interval = box_witness[state_index - 1];
    return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness_interval.lo, BinaryOpExpression::GE);
}

std::unique_ptr<Expression> SelectionRefinement::create_upper_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness) const {
    const VariableIndex_type state_index = var.get_variable_index();
    const auto witness_interval = box_witness[state_index - 1];
    return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness_interval.hi, BinaryOpExpression::LT);
}
std::unique_ptr<Expression> SelectionRefinement::create_split(const LValueExpression& var, ActionLabel_type action, const std::vector<veritas::Interval> box_witness, const StateBase& concretization) const {
    const VariableIndex_type state_index = var.get_variable_index();

    const auto witness_interval = box_witness[state_index - 1];
    auto value_concrete = concretization.get_value(var);
    const auto value_concrete_float = value_concrete->evaluate_floating_const();
    // PLAJA_ASSERT(value_concrete_float < witness_interval.lo or value_concrete_float >= witness_interval.hi)

    // independent of split position, the added constraint expresses "within witness split"

    std::unique_ptr<Expression> split_point(nullptr);
    PLAJA_ASSERT(splitPoint == SelectionRefinement::NearWitness)
    // std::cout << "State Index: " << state_index << ", interval " << witness_interval << "Val: " << value_concrete_float << std::endl;
    if (value_concrete_float < witness_interval.lo) {
        return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness_interval.lo, BinaryOpExpression::GE);
    } else if (value_concrete_float >= witness_interval.hi) {
        return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness_interval.hi, BinaryOpExpression::LT);
    } 
    PLAJA_ABORT;
}

#endif

std::unique_ptr<Expression> SelectionRefinement::create_split(const LValueExpression& var, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const {

    const VariableIndex_type state_index = var.get_variable_index();
    PLAJA_ASSERT(witness.non_equal(concretization, state_index))

    const auto value_witness = witness.get_value(var);
    auto value_concrete = concretization.get_value(var);
    const auto value_witness_float = value_witness->evaluate_floating_const();
    const auto value_concrete_float = value_concrete->evaluate_floating_const();
    PLAJA_ASSERT(PLAJA_FLOATS::non_equal(value_witness_float, value_concrete_float, PREDICATE::predicateSplitPrecision))

    // independent of split position, the added constraint expresses "within witness split"

    std::unique_ptr<Expression> split_point(nullptr);

    if (concretization.lt(witness, state_index)) {

        switch (splitPoint) {

            case SplitPoint::NearConcretization: {
                return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), concretization.get_value(var), BinaryOpExpression::GT);
            }

            case SplitPoint::NearWitness: {
                return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness.get_value(var), BinaryOpExpression::GE);
            }

            case SplitPoint::Middle: {
                const auto middle_point = (value_concrete_float + value_witness_float) / 2;
                split_point = var.is_floating() ? PLAJA_EXPRESSION::make_real(middle_point) : PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(std::ceil(middle_point)));
                break;
            }

            case SplitPoint::Random: {
                if (var.is_floating()) {
                    split_point = PLAJA_EXPRESSION::make_real(rng->sample_float_inverse(concretization.get_float(state_index), witness.get_float(state_index)));
                } else {
                    split_point = PLAJA_EXPRESSION::make_int(rng->sample_signed(concretization.get_int(state_index) + 1, witness.get_int(state_index)));
                }
                break;
            }

            case SplitPoint::WsSearch: {
                auto ws_search = SELECTION_REFINEMENT::SplitBinarySearch<false>(*policy, var, action, witness, value_concrete_float);
                split_point = ws_search.search_split();
                break;
            }

            case SplitPoint::VarSelEps: {
                split_point = varSelection->get_last_lower_perturbation_bound(state_index);
                PLAJA_ASSERT(value_witness_float >= split_point->evaluate_floating_const())
                break;
            }

            default: PLAJA_ABORT

        }

        if (PLAJA_FLOATS::equal(split_point->evaluate_floating_const(), value_concrete_float, PREDICATE::predicateSplitPrecision)) { // fall-back to near-concretization
            return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), std::move(value_concrete), BinaryOpExpression::GT);
        }
        return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), std::move(split_point), BinaryOpExpression::GE);

    } else {
        PLAJA_ASSERT(witness.lt(concretization, state_index))

        switch (splitPoint) {

            case SplitPoint::NearConcretization: {
                return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), concretization.get_value(var), BinaryOpExpression::LT);
            }

            case SplitPoint::NearWitness: {
                return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), witness.get_value(var), BinaryOpExpression::LE);
            }

            case SplitPoint::Middle: {
                const auto middle_point = (value_concrete_float + value_witness_float) / 2;
                split_point = var.is_floating() ? PLAJA_EXPRESSION::make_real(middle_point) : PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(std::ceil(middle_point)));
                break;
            }

            case SplitPoint::Random: {
                if (var.is_floating()) {
                    split_point = PLAJA_EXPRESSION::make_real(rng->sample_float(witness.get_float(state_index), concretization.get_float(state_index)));
                } else {
                    split_point = PLAJA_EXPRESSION::make_int(rng->sample_signed(witness.get_int(state_index), concretization.get_int(state_index) - 1));
                }
                break;
            }

            case SplitPoint::WsSearch: {
                auto ws_search = SELECTION_REFINEMENT::SplitBinarySearch<true>(*policy, var, action, witness, value_concrete_float);
                split_point = ws_search.search_split();
                break;
            }

            case SplitPoint::VarSelEps: {
                split_point = varSelection->get_last_upper_perturbation_bound(state_index);
                PLAJA_ASSERT(value_witness_float <= split_point->evaluate_floating_const())
                break;
            }

            default: PLAJA_ABORT

        }

        if (PLAJA_FLOATS::equal(split_point->evaluate_floating_const(), value_concrete_float, PREDICATE::predicateSplitPrecision)) { // fall-back to near-concretization
            return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), std::move(value_concrete), BinaryOpExpression::LT);
        } // else:
        return BinaryOpExpression::construct_bound(var.deepCopy_Exp(), std::move(split_point), BinaryOpExpression::LE);
    }

}

/**********************************************************************************************************************/

void SelectionRefinement::add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization) const {
    if (stats) { stats->inc_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits); }
    // var <= val
    std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::LE));
    split->set_left(var.deepCopy_Exp());
    split->set_right(concretization.get_value(var));
    split->determine_type();
    result.add_refinement_predicate_wp(split->deepCopy_Exp());
    // var >= val
    split->set_op(BinaryOpExpression::GE);
    result.add_refinement_predicate_wp(std::move(split));
}
#ifdef USE_VERITAS
void SelectionRefinement::add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const std::vector<veritas::Interval> box_witness) const {
    if (stats) { stats->inc_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits); }
    const auto state_index = var.get_variable_index();
    const auto interval = box_witness[state_index - 1];
    std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::LE));
    // var < lb
    split->set_left(var.deepCopy_Exp());
    split->set_right(var.is_floating() ? PLAJA_EXPRESSION::make_real(interval.lo) : PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(interval.lo)));
    split->set_op(BinaryOpExpression::LT);
    split->determine_type();
    result.add_refinement_predicate_wp(split->deepCopy_Exp());
    // var >= ub
    split->set_right(var.is_floating() ? PLAJA_EXPRESSION::make_real(interval.hi) : PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(interval.hi)));
    split->set_op(BinaryOpExpression::GE);
    result.add_refinement_predicate_wp(std::move(split));
}

void SelectionRefinement::add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization, const std::vector<veritas::Interval> box_witness) const {
    if (stats) { stats->inc_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits); }
    const auto state_index = var.get_variable_index();
    const auto interval = box_witness[state_index - 1];
    if (state_index < concretization.get_int_state_size()) {
        const auto val = concretization.get_int(state_index);
        // std::cout << "State Index: " << state_index << ", interval " << interval << "Val: " << val << std::endl;
        if (val < interval.lo) { 
            std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::LT));
            split->set_left(var.deepCopy_Exp());
            split->set_right(PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(interval.lo)));
            split->determine_type();
            result.add_refinement_predicate_wp(std::move(split));
        } else if (val >= interval.hi) {
            std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::GE));
            split->set_left(var.deepCopy_Exp());
            split->set_right(PLAJA_EXPRESSION::make_int(static_cast<PLAJA::integer>(interval.hi)));
            split->determine_type();
            result.add_refinement_predicate_wp(std::move(split));
        }
    } else {
        const auto val = concretization.get_float(state_index);
        if (val < interval.lo) { 
            std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::LT));
            split->set_left(var.deepCopy_Exp());
            split->set_right(PLAJA_EXPRESSION::make_real(interval.lo));
            split->determine_type();
            result.add_refinement_predicate_wp(std::move(split));
        } else if (val >= interval.hi) {
            std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::GE));
            split->set_left(var.deepCopy_Exp());
            split->set_right(PLAJA_EXPRESSION::make_real(interval.hi));
            split->determine_type();
            result.add_refinement_predicate_wp(std::move(split));
        }
    }
}
#endif

void SelectionRefinement::add_entailed_refinement_preds(SpuriousnessResult& result, const LValueExpression& var, const StateBase& concretization, const StateBase& witness) const {
    if (stats) { stats->inc_attr_unsigned(PLAJA::StatsUnsigned::EntailedSplits); }
    const auto state_index = var.get_variable_index();
    PLAJA_ASSERT(concretization.non_equal(witness, state_index))
    if (concretization.lt(witness, state_index)) {
        // var <= val
        std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::LE));
        split->set_left(var.deepCopy_Exp());
        split->set_right(concretization.get_value(var));
        split->determine_type();
        result.add_refinement_predicate_wp(std::move(split));
    } else {
        PLAJA_ASSERT(witness.lt(concretization, state_index))
        // var >= val
        std::unique_ptr<BinaryOpExpression> split(new BinaryOpExpression(BinaryOpExpression::GE));
        split->set_left(var.deepCopy_Exp());
        split->set_right(concretization.get_value(var));
        split->determine_type();
        result.add_refinement_predicate_wp(std::move(split));
    }
}

std::pair<bool, std::unordered_set<VariableIndex_type>> SelectionRefinement::add_entailed_refinement_preds(SpuriousnessResult& result, const StateBase& concretization) const {

    if (entailmentMode == EntailmentMode::None) { return { false, {} }; }

    const auto entailed_vars = result.compute_entailments_set(concretization, true);

    if (entailed_vars.empty()) { return { false, {} }; }

    for (auto it = policyInterface->init_input_feature_iterator(true); !it.end(); ++it) {
        if (entailed_vars.count(it.get_input_feature_index())) { add_entailed_refinement_preds(result, *it.get_input_feature_expression(), concretization); }
    }

    return { entailmentMode == EntailmentMode::EntailmentOnly and result.has_new_predicates(), std::move(entailed_vars) };

}

/**********************************************************************************************************************/

void SelectionRefinement::witness_split_random_one(SpuriousnessResult& result, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const {
    auto candidates = witness_split_candidates(witness, concretization);
    // add exactly one candidate
    auto index = rng->index(candidates.size());
    const auto end = candidates.end();
    for (auto it = candidates.begin(); it != end; ++it) {
        if (index-- == 0) {
            add_refinement_preds(result, { *it }, action, witness, concretization);
            return;
        }
    }
    PLAJA_ABORT
}

void SelectionRefinement::witness_split_random(SpuriousnessResult& result, ActionLabel_type action, const StateBase& witness, const StateBase& concretization) const {
    auto candidates = witness_split_candidates(witness, concretization);

    if (candidates.size() > 1) { // nothing to be done if there is only a single candidate
        const auto* fall_back_var = candidates.front();
        auto fall_back_var_index = rng->index(candidates.size()); // candidate to be used in case all other candidates are randomly dropped

        auto end = candidates.cend();
        for (auto it = candidates.cbegin(); it != end;) {
            if (fall_back_var_index-- == 0) { fall_back_var = *it; }
            if (rng->flag()) { it = candidates.erase(it); }
            else { ++it; }
        }

        PLAJA_ASSERT(fall_back_var)
        if (candidates.empty()) {
            add_refinement_preds(result, { fall_back_var }, action, witness, concretization);
            return;
        }
    }

    add_refinement_preds(result, candidates, action, witness, concretization);
}

void SelectionRefinement::perform_non_ws_refinement(SpuriousnessResult& result, const StateBase& concretization) const {

    const auto finished_entails = add_entailed_refinement_preds(result, concretization);
    if (finished_entails.first) { return; }
    const auto& entailed_vars = finished_entails.second;

    const auto& predicates = result.get_predicates();
    std::unique_ptr<std::pair<StateValues, StateValues>> state_bounds(nullptr);

    if (splitPoint == GlobalSplitPaState) { // TODO might use/or-not-use in fall-back mode.
        const auto& pa_state = result.get_pa_state(result.get_spurious_prefix_length());
        state_bounds = std::make_unique<std::pair<StateValues, StateValues>>(pa_state.compute_bounds(policyInterface->get_model_info()));
        predicates.set_interval_for_propose_split(&state_bounds->first, &state_bounds->second);
    }

    for (auto it = policyInterface->init_input_feature_iterator(true); !it.end(); ++it) {

        const auto* var = it.get_input_feature_expression();
        PLAJA_ASSERT(PLAJA_UTILS::is_dynamic_ptr_type<VariableExpression>(var)) // sanity
        const auto state_index = it.get_input_feature_index();
        PLAJA_ASSERT(var->get_variable_index() == state_index) // sanity

        if (entailed_vars.count(state_index)) { continue; }

        if (mode == Mode::ConcretizationExclusion) {

            auto value = concretization.get_value(*var);
            /* v < state(v) */
            result.add_refinement_predicate_wp(BinaryOpExpression::construct_bound(var->deepCopy_Exp(), value->deepCopy_Exp(), BinaryOpExpression::LT));
            /* v > state(v) */
            result.add_refinement_predicate_wp(BinaryOpExpression::construct_bound(var->deepCopy_Exp(), std::move(value), BinaryOpExpression::GT));

        } else {
            PLAJA_ASSERT(mode == Mode::All)
            PLAJA_ASSERT(splitPoint == SplitPoint::BinarySplit or splitPoint == SplitPoint::GlobalSplitPaState)

            // TODO might want option to add split only locally, i.p., for GlobalSplitPaState, GlobalSplitWs
            auto split = predicates.propose_split(*var, result.get_used_predicates(result.get_spurious_prefix_length()), result.get_pa_state_if_local(result.get_spurious_prefix_length()));
            if (split) { result.add_refinement_predicate(std::move(split)); }
        }

    }

    predicates.set_interval_for_propose_split();

}

/**********************************************************************************************************************/
#ifdef USE_VERITAS
void SelectionRefinement::add_refinement_preds(class SpuriousnessResult& result, ActionLabel_type action, std::vector<veritas::Interval> box_witness, const StateBase* concretization) const {
    PLAJA_ASSERT(needs_interval_witness())
    add_refinement_preds(result, witness_split_candidates(box_witness, *concretization), action, box_witness, *concretization);
    return; 
}
#endif

void SelectionRefinement::add_refinement_preds(class SpuriousnessResult& result, ActionLabel_type action, const StateBase* witness, const StateBase* concretization) const {

    PLAJA_ASSERT(concretization) // We can always obtain a concretization from z3.
    PLAJA_ASSERT(not concretization or not policy->is_chosen(*concretization, action, PLAJA_NN::argMaxPrecision))
    PLAJA_ASSERT(not witness or not concretization or *witness != *concretization)
    PLAJA_ASSERT(not witness or policy->is_chosen(*witness, action, PLAJA_NN::argMaxPrecision))

    /* Do need no witness. */
    if (not needs_witness()) {
        perform_non_ws_refinement(result, *concretization);
        return;
    }

    /* Fall back. */
    if (not witness) {
        if (stats) { stats->inc_attr_unsigned(PLAJA::StatsUnsigned::WsFallBack); }
        perform_non_ws_refinement(result, *concretization);
        return;
    }

    /* Perform witness-based refinement. */

    update_ws_stats(*witness, *concretization);

    switch (mode) {
        case WsAll: {
            add_refinement_preds(result, witness_split_candidates(*witness, *concretization), action, *witness, *concretization);
            return;
        }
        case WsMaxAbs: {
            SELECTION_REFINEMENT::SplitVarsSelection::witness_split_priority<true, false>(result, *this, action, *witness, *concretization);
            return;
        }
        case WsMaxRel: {
            SELECTION_REFINEMENT::SplitVarsSelection::witness_split_priority<true, true>(result, *this, action, *witness, *concretization);
            return;
        }
        case WsMinAbs: {
            SELECTION_REFINEMENT::SplitVarsSelection::witness_split_priority<false, false>(result, *this, action, *witness, *concretization);
            return;
        }
        case WsMinRel: {
            SELECTION_REFINEMENT::SplitVarsSelection::witness_split_priority<false, true>(result, *this, action, *witness, *concretization);
            return;
        }
        case WsVarSel: {
            add_refinement_preds(result, varSelection->compute_relevant_vars(*witness, *concretization, action), action, *witness, *concretization);
            return;
        }
        case WsSensitiveOne: {
            add_refinement_preds(result, { varSelection->compute_most_sensitive_variable(*witness, *concretization) }, action, *witness, *concretization);
            return;
        }
        case WsSmtSearchC: {
            add_refinement_preds(result, varSelection->compute_decision_boundary(*witness, *concretization, action), action, *witness, *concretization);
            return;
        }
        case WsSmtSearch:
        case WsSmtSearchPa: {

            // Use entailment ?
            PLAJA_ASSERT(entailmentMode != EntailmentMode::UseEntailment)
            if (entailmentMode == EntailmentMode::EntailmentOnly) {
                auto entailed_vars = result.compute_entailments_set(*concretization, true);

                if (not entailed_vars.empty()) {
                    for (auto it = policyInterface->init_input_feature_iterator(true); !it.end(); ++it) {
                        const auto state_index = it.get_input_feature_index();
                        if (entailed_vars.count(it.get_input_feature_index()) and concretization->non_equal(*witness, state_index)) { add_entailed_refinement_preds(result, *it.get_input_feature_expression(), *concretization, *witness); }
                    }
                }

                if (result.has_new_predicates()) { return; }

            }

            const auto split_vars = varSelection->compute_decision_boundary(*witness, mode == WsSmtSearchPa ? &result.get_pa_state(result.get_spurious_prefix_length()) : nullptr, action);

            // TODO If perturbation bound is (due to) abstract state boundary we might not want to add the corresponding constraint as new predicate -> no "actual" policy decision boundary.
            //  This especially applies to wp-computation based refinement.

            for (const auto* var: split_vars) {
                const auto state_index = var->get_variable_index();
                result.add_refinement_predicate_wp(BinaryOpExpression::construct_bound(var->deepCopy_Exp(), varSelection->get_last_lower_perturbation_bound(state_index), BinaryOpExpression::GE));
                result.add_refinement_predicate_wp(BinaryOpExpression::construct_bound(var->deepCopy_Exp(), varSelection->get_last_upper_perturbation_bound(state_index), BinaryOpExpression::LE));
            }

            return;
        }
        case WsRandomOne: {
            witness_split_random_one(result, action, *witness, *concretization);
            return;
        }
        case WsRandom: {
            witness_split_random(result, action, *witness, *concretization);
            return;
        }
        default: { PLAJA_ABORT } // Non-witness based cases should be covered if if condition above.
    }

    PLAJA_ABORT

}

