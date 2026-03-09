//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "model_smt.h"
#include "../../../parser/ast/expression/expression.h"
#include "../../../parser/ast/expression/non_standard/predicates_expression.h"
#include "../../../parser/ast/expression/expression_utils.h"
#include "../../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../../parser/ast/model.h"
#include "../../../parser/visitor/extern/ast_specialization.h"
#include "../../../parser/visitor/extern/to_normalform.h"
#include "../../factories/configuration.h"
#include "../../information/input_feature_to_jani.h"
#include "../../information/jani_2_interface.h"
#include "../../information/model_information.h"
#include "../../information/property_information.h"
#include "../../states/state_values.h"
#include "../../successor_generation/successor_generator_c.h"
#include "../successor_generation/op_applicability.h"
#include "smt_solver.h"
#include "smt_constraint.h"

ModelSmt::ModelSmt(std::shared_ptr<PLAJA::SmtContext>&& context, const PLAJA::Configuration& config): // NOLINT(modernize-pass-by-value)
    context(std::move(context))
    , model(nullptr)
    , modelInfo(nullptr)
    , interface(nullptr)
    , predicates(nullptr)
    , cachedOpApp(nullptr)
    , ignoreLocations(false)
    , maxStep(1) {

    if (config.has_sharable_const(PLAJA::SharableKey::MODEL)) {
        model = config.get_sharable_const<Model>(PLAJA::SharableKey::MODEL);
    } else { PLAJA_ASSERT(config.has_sharable_const(PLAJA::SharableKey::PROP_INFO)) }

    if (config.has_sharable_const(PLAJA::SharableKey::PROP_INFO)) {
        const auto* prop_info = config.get_sharable_const<PropertyInformation>(PLAJA::SharableKey::PROP_INFO);
        if (not model) { model = prop_info->get_model(); }
        else { PLAJA_ASSERT(model == prop_info->get_model()) }

        interface = prop_info->get_interface();
        set_start(prop_info->get_start() ? prop_info->get_start()->deepCopy_Exp() : nullptr);
        set_reach(prop_info->get_reach() ? prop_info->get_reach()->deepCopy_Exp() : nullptr);
        if constexpr (PLAJA_GLOBAL::enableTerminalStateSupport) { set_non_terminal(prop_info->get_terminal() ? prop_info->get_terminal()->deepCopy_Exp() : nullptr, true); }

        predicates = prop_info->get_predicates();
    }

    modelInfo = &model->get_model_information();

    // only use locations if there is more than one location state?
    if (not ignoreLocations) {
        ignoreLocations = true;
        for (auto loc_it = modelInfo->locationIndexIterator(); !loc_it.end(); ++loc_it) {
            if (loc_it.range() > 1) {
                ignoreLocations = false;
                break;
            }
        }
    }

    if (interface) { set_interface(interface); } // Reset to properly trigger computation of extended structures.

    if (config.has_sharable(PLAJA::SharableKey::SUCC_GEN)) {
        successorGenerator = config.get_sharable<SuccessorGeneratorC>(PLAJA::SharableKey::SUCC_GEN);
        PLAJA_ASSERT(&successorGenerator->get_model() == model)
    } else {
        successorGenerator = config.set_sharable(PLAJA::SharableKey::SUCC_GEN, std::make_shared<SuccessorGeneratorC>(config, *model));
    }

}

ModelSmt::~ModelSmt() = default;

/**********************************************************************************************************************/

void ModelSmt::set_interface(const Jani2Interface* new_interface) {

    PLAJA_ASSERT(new_interface)

    PLAJA_ASSERT_EXPECT(not interface or new_interface == interface)

    interface = new_interface;

    // Cache input locations.
    for (InputIndex_type input_index = 0; input_index < interface->get_num_input_features(); ++input_index) {
        const auto* input_feature = interface->get_input_feature(input_index);
        if (input_feature->is_location()) { inputLocations.push_back(input_feature->stateIndex); }
    }

    PLAJA_ASSERT(inputLocations.empty() or not ignoreLocations)

}

void ModelSmt::set_start(std::unique_ptr<Expression>&& start_ptr) { start = std::move(start_ptr); }

void ModelSmt::set_reach(std::unique_ptr<Expression>&& reach_ptr) { reach = std::move(reach_ptr); }

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void ModelSmt::set_non_terminal(std::unique_ptr<Expression>&& terminal, bool negate) {
    PLAJA_ASSERT(PLAJA_GLOBAL::enableTerminalStateSupport)
    if (terminal) {
        auto terminal_state_con = StateConditionExpression::transform(std::move(terminal));
        PLAJA_ASSERT(terminal_state_con->get_size_loc_values() == 0) // Not supported.
        nonTerminal = terminal_state_con->set_constraint();
        if (negate) { TO_NORMALFORM::negate(nonTerminal); }
        if constexpr (not PLAJA_GLOBAL::reachMayBeTerminal) {
            std::list<std::unique_ptr<Expression>> reach_non_terminal_conjuncts;
            reach_non_terminal_conjuncts.push_back(nonTerminal->deepCopy_Exp());
            reach_non_terminal_conjuncts.push_back(reach->deepCopy_Exp());
            reachNonTerminal = TO_NORMALFORM::construct_conjunction(std::move(reach_non_terminal_conjuncts));
        } else { reachNonTerminal = nullptr; }
    } else {
        nonTerminal = nullptr;
        reachNonTerminal = nullptr;
    }
    nonTerminalSmt.clear();

}

#else

void ModelSmt::set_non_terminal(std::unique_ptr<Expression>&&, bool) { PLAJA_ABORT }

#endif

/**********************************************************************************************************************/

std::size_t ModelSmt::get_number_automata_instances() const { return get_model().get_number_automataInstances(); }

std::size_t ModelSmt::get_array_size(VariableID_type var_id) const { return get_model_info().get_array_size(var_id); }

bool ModelSmt::initial_state_is_subsumed_by_start() const { return start and start->evaluate_integer(modelInfo->get_initial_values()); }

const Expression* ModelSmt::get_terminal() const { return successorGenerator->get_terminal_state_condition(); }

std::size_t ModelSmt::get_number_predicates() const {
    PLAJA_ASSERT(predicates)
    return predicates->get_number_predicates();
}

const Expression* ModelSmt::get_predicate(std::size_t index) const {
    PLAJA_ASSERT(predicates)
    return predicates->get_predicate(index);
}

bool ModelSmt::predicate_is_bound(std::size_t index) const { return predicates->predicate_is_bound(index); }

/**********************************************************************************************************************/

const ActionOp& ModelSmt::get_action_op_concrete(ActionOpID_type op_id) const { return successorGenerator->get_action_op(op_id); }

/* */

void ModelSmt::share_op_applicability(std::shared_ptr<OpApplicability> op_app) const { cachedOpApp = std::move(op_app); }

void ModelSmt::set_self_applicability(bool value) const {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) { if (cachedOpApp) { cachedOpApp->set_self_applicability(value); } }
}

void ModelSmt::unset_self_applicability() const { if (cachedOpApp) { cachedOpApp->unset_self_applicability(); } }

void ModelSmt::add_self_applicability(bool value) const {
    if constexpr (PLAJA_GLOBAL::enableApplicabilityCache) {
        if (not cachedOpApp) { cachedOpApp = std::make_unique<OpApplicability>(); }
        cachedOpApp->set_self_applicability(value);
    }
}
/**********************************************************************************************************************/

std::unique_ptr<Expression> ModelSmt::optimize_expression(std::unique_ptr<Expression> expr, PLAJA::SmtSolver& solver, bool prepare, bool check_entailment, bool prune_conjuncts, bool optimize_disjuncts) const {
    PLAJA_LOG_DEBUG("Optimizing expression for SMT encoding ...")

    auto conjuncts = TO_NORMALFORM::split_conjunction(std::move(expr), false);
    PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("%i conjuncts ...", conjuncts.size()))

    std::list<std::unique_ptr<Expression>> conjuncts_processed;

    for (auto& conjunct: conjuncts) {

        if (prepare) { TO_NORMALFORM::to_dnf(conjunct); }
        auto disjuncts = TO_NORMALFORM::split_disjunction(std::move(conjunct), false);
        PLAJA_ASSERT(not disjuncts.empty())

        /**************************************************************************************************************/
        PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("%i disjuncts in conjunct ...", disjuncts.size()))

        bool entailed(false);

        for (auto it_dis = disjuncts.begin(); it_dis != disjuncts.end();) {
            if (disjuncts.size() == 1 and not prune_conjuncts) { break; }

            const auto constraint = to_smt(**it_dis, 0);

            const auto rlt = check_entailment ? solver.check_entailment(*constraint, true) : solver.check(*constraint, true); // Expecting many simple constraints, so pre-check makes sense.
            PLAJA_ASSERT(not solver.is_unknown(rlt))

            if (rlt == PLAJA::SmtSolver::Sat) {

                if (optimize_disjuncts) {
                    auto& disjunct = *it_dis;
                    auto sub_disjuncts = TO_NORMALFORM::split_conjunction(std::move(disjunct), false);
                    PLAJA_ASSERT(not sub_disjuncts.empty())

                    for (auto it_dis_sub = sub_disjuncts.begin(); it_dis_sub != sub_disjuncts.end();) {
                        if (sub_disjuncts.size() <= 1) { break; }
                        solver.push();
                        const auto constraint_sub = to_smt(**it_dis_sub, 0);
                        constraint_sub->add_negation_to_solver(solver);
                        const auto rlt_sub = solver.pre_and_check();
                        PLAJA_ASSERT(not solver.is_unknown(rlt_sub))
                        PLAJA_ASSERT(rlt_sub != PLAJA::SmtSolver::Entailed)
                        if (rlt_sub == PLAJA::SmtSolver::UnSat) { it_dis_sub = sub_disjuncts.erase(it_dis_sub); }
                        else { ++it_dis_sub; }
                        solver.pop();
                    }

                    PLAJA_ASSERT(not sub_disjuncts.empty())
                    disjunct = TO_NORMALFORM::construct_conjunction(std::move(sub_disjuncts));
                }

                ++it_dis;

            } else if (rlt == PLAJA::SmtSolver::UnSat) {

                it_dis = disjuncts.erase(it_dis);

            } else {
                PLAJA_ASSERT(rlt == PLAJA::SmtSolver::Entailed)

                if (prune_conjuncts) {
                    entailed = true;
                    disjuncts.clear();
                } else {
                    disjuncts.resize(1);
                }

                break;
            }
        }

        PLAJA_LOG_DEBUG(PLAJA_UTILS::string_f("%i disjuncts after preprocessing ...", disjuncts.size()))
        /**************************************************************************************************************/

        PLAJA_ASSERT(prune_conjuncts or not disjuncts.empty())

        if (entailed) { continue; }

        if (disjuncts.empty()) { return PLAJA_EXPRESSION::make_bool(false); }

        conjuncts_processed.push_back(TO_NORMALFORM::construct_disjunction(std::move(disjuncts)));
    }

    PLAJA_ASSERT(prune_conjuncts or conjuncts.size() == conjuncts_processed.size())

    return conjuncts_processed.empty() ? PLAJA_EXPRESSION::make_bool(true) : TO_NORMALFORM::construct_conjunction(std::move(conjuncts_processed));
}

std::unique_ptr<Expression> ModelSmt::optimize_expression(const Expression& expr, PLAJA::SmtSolver& solver, bool prepare, bool check_entailment, bool prune_conjuncts, bool optimize_disjuncts) const {
    return optimize_expression(expr.deepCopy_Exp(), solver, prepare, check_entailment, prune_conjuncts, optimize_disjuncts);
}

/* */

#ifdef ENABLE_TERMINAL_STATE_SUPPORT

void ModelSmt::add_terminal(PLAJA::SmtSolver& solver, StepIndex_type step) const {
    if (nonTerminalSmt.empty()) { return; }
    PLAJA_ASSERT(step < nonTerminalSmt.size())
    nonTerminalSmt[step]->add_negation_to_solver(solver);
}

void ModelSmt::exclude_terminal(PLAJA::SmtSolver& solver, StepIndex_type step) const {
    if (nonTerminalSmt.empty()) { return; }
    PLAJA_ASSERT(step < nonTerminalSmt.size())
    nonTerminalSmt[step]->add_to_solver(solver);
}

#else

void ModelSmt::add_terminal(PLAJA::SmtSolver&, StepIndex_type) const {}

void ModelSmt::exclude_terminal(PLAJA::SmtSolver&, StepIndex_type) const {}

#endif
