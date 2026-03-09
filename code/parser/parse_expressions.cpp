//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "parser.h"
#include "../exception/parser_exception.h"
#include "../option_parser/option_parser.h"
#include "ast/expression/non_standard/constant_array_access_expression.h"
#include "ast/expression/non_standard/let_expression.h"
#include "ast/expression/non_standard/location_value_expression.h"
#include "ast/expression/non_standard/objective_expression.h"
#include "ast/expression/non_standard/predicate_abstraction_expression.h"
#include "ast/expression/non_standard/predicates_expression.h"
#include "ast/expression/non_standard/problem_instance_expression.h"
#include "ast/expression/non_standard/state_condition_expression.h"
#include "ast/expression/non_standard/state_values_expression.h"
#include "ast/expression/non_standard/states_values_expression.h"
#include "ast/expression/non_standard/variable_value_expression.h"
#include "ast/expression/array_access_expression.h"
#include "ast/expression/array_constructor_expression.h"
#include "ast/expression/array_value_expression.h"
#include "ast/expression/binary_op_expression.h"
#include "ast/expression/bool_value_expression.h"
#include "ast/expression/constant_expression.h"
#include "ast/expression/derivative_expression.h"
#include "ast/expression/distribution_sampling_expression.h"
#include "ast/expression/expectation_expression.h"
#include "ast/expression/filter_expression.h"
#include "ast/expression/integer_value_expression.h"
#include "ast/expression/ite_expression.h"
#include "ast/expression/path_expression.h"
#include "ast/expression/qfied_expression.h"
#include "ast/expression/real_value_expression.h"
#include "ast/expression/state_predicate_expression.h"
#include "ast/expression/unary_op_expression.h"
#include "ast/expression/variable_expression.h"
#include "ast/expression/free_variable_expression.h"
#include "ast/non_standard/free_variable_declaration.h"
#include "ast/non_standard/reward_accumulation.h"
#include "ast/automaton.h"
#include "ast/location.h"
#include "ast/model.h"
#include "ast/property_interval.h"
#include "ast/reward_bound.h"
#include "ast/reward_instant.h"
#include "parser_utils.h"

namespace PARSER { bool allow_let = false; } // TODO for now to only allow "let" construct within start conditions and goal potetial


std::unique_ptr<PropertyExpression> Parser::parse_PropertyExpression(const nlohmann::json& input, bool within_filter) { // NOLINT(misc-no-recursion)

    if (input.is_object() && input.contains(JANI::OP)) { // note: non-expression property expressions always contain JANI::OP
        const auto& op_j = input.at(JANI::OP);
        PARSER::check_string(op_j, input);
        if (JANI::FILTER == op_j) { return parse_FilterExpression(input); }
        if (within_filter) { // within filter further property expressions are possible:
            if (ExpectationExpression::str_to_exp_qualifier(op_j)) { return parse_ExpectationExpression(input); }
            if (PathExpression::str_to_path_op(op_j)) { return parse_PathExpression(input); }
            if (QfiedExpression::str_to_qfier(op_j)) { return parse_QfiedExpression(input); }
            if (StatePredicateExpression::str_to_op(op_j)) { // special case STATE PREDICATE:
                PARSER::check_for_unexpected_element(input, 1);
                return std::make_unique<StatePredicateExpression>(*StatePredicateExpression::str_to_op(op_j));
            }
        }
        if (PAExpression::get_op_string() == op_j) { return parse_PAExpression(input); } // predicate abstraction
        if (ProblemInstanceExpression::get_op_string() == op_j) { return parse_problem_instance_expression(input); }
    }

    // try to parse as expression (may contain op or not)
    try {
        /*if (within_filter)*/ return parse_Expression(input); // expression outside of filter are not supported though
    } catch (ParserException& parserException) {} // failed, catch to show that error occurred while parsing *property* expression

    PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure);
}

std::unique_ptr<Expression> Parser::parse_Expression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)

    if (input.is_boolean()) { return std::make_unique<BoolValueExpression>(input); }
    if (input.is_number_integer()) { return std::make_unique<IntegerValueExpression>(input); }
    if (input.is_number_float()) { return std::make_unique<RealValueExpression>(RealValueExpression::NONE_C, input); }

    if (input.is_string()) {
        if (constants.count(input)) { return parse_ConstantExpression(input); }
        if (globalVars.count(input) or localVars.count(input)) { return parse_VariableExpression(input); }
        if (freeVars.count(input)) {
            const auto* free_var = freeVars.at(input);
            return std::make_unique<FreeVariableExpression>(input, free_var->get_id(), free_var->get_type());
        }
        if (DistributionSamplingExpression::str_to_distribution_type(input)) { return parse_DistributionSamplingExpression(input); }
    }

    if (input.is_object() and input.contains(JANI::OP)) {
        const auto& op_j = input.at(JANI::OP);
        PARSER::check_string(op_j, input);
        if (BinaryOpExpression::str_to_binary_op(op_j)) { return parse_BinaryOpExpression(input); }
        if (UnaryOpExpression::str_to_unary_op(op_j)) { return parse_UnaryOpExpression(input); }
        // consider non-enumerated ops:
        if (JANI::AA == op_j) {
            auto const_aa_exp = parse_constant_array_access_expression_if(input);
            if (const_aa_exp) { return const_aa_exp; }
            return parse_array_access_expression(input);
        }
        if (JANI::AC == op_j) { return parse_array_constructor_expression(input); }
        if (JANI::AV == op_j) { return parse_array_value_expression(input); }
        if (JANI::DER == op_j) { return parse_DerivativeExpression(input); }
        if (JANI::ITE == op_j) { return parse_ITE_Expression(input); }
            // non-standard (currently explicitly & exclusively used in PA expression)
        else if (PARSER::allow_let and LetExpression::get_op_string() == op_j) { return parse_LetExpression(input); }
        if (StateConditionExpression::get_op_string() == op_j) { return parse_StateConditionExpression(input); }
        // if (STATE_VALUES_EXPRESSION::opString == op_j) { return parse_StateValuesExpression(input); }
        // if (STATES_VALUES_EXPRESSION::opString == op_j) { return parse_StatesValuesExpression(input); }
    }

    if (input.is_object() && input.contains(JANI::CONSTANT)) { return parse_RealValueExpression(input); }

    PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure);
}

std::unique_ptr<LValueExpression> Parser::parse_LValueExpression(const nlohmann::json& input) {
    if (input.is_string()) { return parse_VariableExpression(input); }
    if (input.is_object() and input.contains(JANI::OP)) {
        const nlohmann::json& op_j = input.at(JANI::OP);
        if (op_j.is_string() and op_j == JANI::AA) { return parse_array_access_expression(input); }
    }
    PARSER::throw_parser_exception(input, PARSER::unmatched_jani_structure);
}

std::unique_ptr<ArrayAccessExpression> Parser::parse_array_access_expression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    /* op */
    PARSER::check_op_decoupled(input, JANI::AA);

    std::unique_ptr<ArrayAccessExpression> aa_exp(new ArrayAccessExpression());
    /* exp */
    PARSER::check_contains(input, JANI::EXP);
    // aa_exp->set_accessedArray(parse_Expression(input.at(JANI::EXP)));
    /* Current support. */
    const nlohmann::json& exp_j = input.at(JANI::EXP);
    if (exp_j.is_string()) { aa_exp->set_accessedArray(parse_VariableExpression(exp_j)); }
    else {
        parse_Expression(exp_j); // Do not declare invalid output as not supported.
        PARSER::throw_not_supported(exp_j, input, "only one dimensional static arrays");
    }

    /* index */
    PARSER::check_contains(input, JANI::INDEX);
    aa_exp->set_index(parse_Expression(input.at(JANI::INDEX)));

    PARSER::check_for_unexpected_element(input, 3);
    return aa_exp;
}

std::unique_ptr<ArrayConstructorExpression> Parser::parse_array_constructor_expression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    /* op */
    PARSER::check_op_decoupled(input, JANI::AC);

    std::unique_ptr<ArrayConstructorExpression> ac_exp(new ArrayConstructorExpression());

    /* length */
    PARSER::check_contains(input, JANI::LENGTH);
    ac_exp->set_length(parse_Expression(input.at(JANI::LENGTH)));

    /* var */
    PARSER::check_contains(input, JANI::VAR);
    const nlohmann::json& var_name_j = input.at(JANI::VAR);
    PARSER::check_string(var_name_j, input);
    PARSER::throw_parser_exception_if(globalVars.count(var_name_j) or constants.count(var_name_j) or localVars.count(var_name_j) || freeVars.count(var_name_j), var_name_j, input, PARSER::duplicate_name);

    /* name */
    nlohmann::json free_var_j;
    free_var_j.emplace(JANI::NAME, var_name_j);
    // type
    nlohmann::json type_j;
    type_j.emplace(JANI::BASE, "int");
    type_j.emplace(JANI::LOWER_BOUND, 0);
    type_j.emplace(JANI::UPPER_BOUND, ac_exp->get_length()->to_string());
    free_var_j.emplace(JANI::TYPE, std::move(type_j));
    const std::unique_ptr<FreeVariableDeclaration> var_decl = parse_FreeVariableDeclaration(free_var_j);

    /* exp */
    PARSER::check_contains(input, JANI::EXP);
    freeVars.emplace(var_name_j, var_decl.get()); // Add to free vars for scope of exp.
    ac_exp->set_evalExp(parse_Expression(input.at(JANI::EXP)));
    freeVars.erase(var_name_j); // Remove from scope

    PARSER::check_for_unexpected_element(input, 4);
    return ac_exp;
}

std::unique_ptr<ArrayValueExpression> Parser::parse_array_value_expression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    /* op */
    PARSER::check_op_decoupled(input, JANI::AV);

    std::unique_ptr<ArrayValueExpression> av_exp(new ArrayValueExpression());

    /* elements */
    PARSER::check_contains(input, JANI::ELEMENTS);
    const nlohmann::json& elements_j = input.at(JANI::ELEMENTS);
    PARSER::check_array(elements_j, input);
    const size_t l = elements_j.size();
    PARSER::throw_parser_exception_if(l < 1, elements_j, input, PARSER::at_least_one_element);
    av_exp->reserve(l);
    for (const auto& element_j: elements_j) { av_exp->add_element(parse_Expression(element_j)); }

    PARSER::check_for_unexpected_element(input, 2);
    return av_exp;
}

std::unique_ptr<BinaryOpExpression> Parser::parse_BinaryOpExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    auto binaryOpExp = std::make_unique<BinaryOpExpression>(PARSER::check_op_enum_decoupled<BinaryOpExpression::BinaryOp>(input, BinaryOpExpression::str_to_binary_op));

    PARSER::check_contains(input, JANI::LEFT);
    binaryOpExp->set_left(parse_Expression(input.at(JANI::LEFT)));

    PARSER::check_contains(input, JANI::RIGHT);
    binaryOpExp->set_right(parse_Expression(input.at(JANI::RIGHT)));

    PARSER::check_for_unexpected_element(input, 3);

    return binaryOpExp;
}

std::unique_ptr<ConstantExpression> Parser::parse_ConstantExpression(const nlohmann::json& input) {
    if constexpr (PARSER::decoupledParsing) {
        PARSER::check_string(input);
        PARSER::throw_parser_exception_if(not constants.count(input), input, PARSER::unknown_name);
    }
    const auto* const_decl = constants.at(input);
    return std::make_unique<ConstantExpression>(*const_decl);
}

std::unique_ptr<DerivativeExpression> Parser::parse_DerivativeExpression(const nlohmann::json& input) {
    PARSER::check_object_decoupled(input);

    PARSER::check_op_decoupled(input, JANI::DER);

    const auto var_name = PARSER::parse_string(input, JANI::VAR);
    PARSER::check_for_unexpected_element(input, 2);

    if (globalVars.count(var_name)) { return std::make_unique<DerivativeExpression>(*globalVars.at(var_name)); }

    if (localVars.count(var_name)) { return std::make_unique<DerivativeExpression>(*localVars.at(var_name)); }

    PARSER::throw_parser_exception(var_name, input, PARSER::unknown_name);
}

std::unique_ptr<DistributionSamplingExpression> Parser::parse_DistributionSamplingExpression(const nlohmann::json& input) {
    PARSER::throw_not_supported(input, JANI::DISTRIBUTION);
}

std::unique_ptr<ExpectationExpression> Parser::parse_ExpectationExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    auto expectationExp = std::make_unique<ExpectationExpression>(PARSER::check_op_enum_decoupled<ExpectationExpression::ExpectationQualifier>(input, ExpectationExpression::str_to_exp_qualifier));

    // exp:
    PARSER::check_contains(input, JANI::EXP);
    expectationExp->set_value(parse_Expression(input.at(JANI::EXP)));
    unsigned int object_size = 2; // to check for additional unknown elements

    // accumulate:
    if (input.contains(JANI::ACCUMULATE)) {
        ++object_size;
        expectationExp->set_reward_accumulation(parse_reward_accumulation(input.at(JANI::ACCUMULATE)));
    }

    // reach:
    if (input.contains(JANI::REACH)) {
        ++object_size;
        expectationExp->set_reach(parse_PropertyExpression(input.at(JANI::REACH), true));
    }

    // step-instant:
    if (input.contains(JANI::STEP_INSTANT)) {
        ++object_size;
        expectationExp->set_step_instant(parse_Expression(input.at(JANI::STEP_INSTANT)));
    }

    // time-instant:
    if (input.contains(JANI::TIME_INSTANT)) {
        ++object_size;
        expectationExp->set_time_instant(parse_Expression(input.at(JANI::TIME_INSTANT)));
    }

    // reward-instants:
    if (input.contains(JANI::REWARD_INSTANTS)) {
        ++object_size;
        const nlohmann::json& reward_instants_j = input.at(JANI::REWARD_INSTANTS);
        PARSER::check_array(reward_instants_j, input);
        expectationExp->reserve(reward_instants_j.size());
        for (const auto& reward_instant_j: reward_instants_j) {
            expectationExp->add_reward_instant(parse_RewardInstant(reward_instant_j));
        }
    }

    PARSER::check_for_unexpected_element(input, object_size);

    return expectationExp;
}

std::unique_ptr<FilterExpression> Parser::parse_FilterExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);
    // op:
    PARSER::check_op_decoupled(input, JANI::FILTER);
    // fun:
    PARSER::check_contains(input, JANI::FUN);
    const auto& fun_j = input.at(JANI::FUN);
    PARSER::check_string(fun_j, input);
    auto filter_fun = FilterExpression::str_to_filter_fun(fun_j);
    PARSER::throw_parser_exception_if(not filter_fun, fun_j, input, JANI::FUN);
    std::unique_ptr<FilterExpression> filterExp(new FilterExpression(*filter_fun));
    // values:
    PARSER::check_contains(input, JANI::VALUES);
    filterExp->set_values(parse_PropertyExpression(input.at(JANI::VALUES), true));
    // states:
    PARSER::check_contains(input, JANI::STATES);
    filterExp->set_states(parse_PropertyExpression(input.at(JANI::STATES), true));

    PARSER::check_for_unexpected_element(input, 4);
    return filterExp;
}

std::unique_ptr<ITE_Expression> Parser::parse_ITE_Expression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);
    // op:
    PARSER::check_op_decoupled(input, JANI::ITE);
    //
    std::unique_ptr<ITE_Expression> iteExp(new ITE_Expression());
    // condition:
    PARSER::check_contains(input, JANI::IF);
    iteExp->set_condition(parse_Expression(input.at(JANI::IF)));
    // consequence:
    PARSER::check_contains(input, JANI::THEN);
    iteExp->set_consequence(parse_Expression(input.at(JANI::THEN)));
    // alternative:
    PARSER::check_contains(input, JANI::ELSE);
    iteExp->set_alternative(parse_Expression(input.at(JANI::ELSE)));

    PARSER::check_for_unexpected_element(input, 4);
    return iteExp;
}

std::unique_ptr<PathExpression> Parser::parse_PathExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    auto pathExp = std::make_unique<PathExpression>(PARSER::check_op_enum_decoupled<PathExpression::PathOp>(input, PathExpression::str_to_path_op));

    unsigned int object_size = 1;

    if (not PathExpression::is_binary(pathExp->get_op())) { // unary exp

        PARSER::check_contains(input, JANI::EXP);
        ++object_size;
        pathExp->set_right(parse_PropertyExpression(input.at(JANI::EXP), true));

    } else { // binary exp

        PARSER::check_contains(input, JANI::LEFT);
        ++object_size;
        pathExp->set_left(parse_PropertyExpression(input.at(JANI::LEFT), true));
        PARSER::check_contains(input, JANI::RIGHT);
        ++object_size;
        pathExp->set_right(parse_PropertyExpression(input.at(JANI::RIGHT), true));

    }

    // step-bounds:
    if (input.contains(JANI::STEP_BOUNDS)) {
        ++object_size;
        pathExp->set_step_bounds(parse_PropertyInterval(input.at(JANI::STEP_BOUNDS)));
    }

    // time-bounds:
    if (input.contains(JANI::TIME_BOUNDS)) {
        ++object_size;
        pathExp->set_time_bounds(parse_PropertyInterval(input.at(JANI::TIME_BOUNDS)));
    }

    // reward-instants:
    if (input.contains(JANI::REWARD_BOUNDS)) {
        ++object_size;
        const nlohmann::json& reward_bounds_j = input.at(JANI::REWARD_BOUNDS);
        PARSER::check_array(reward_bounds_j, input);
        pathExp->reserve(reward_bounds_j.size());
        for (const auto& reward_bound_j: reward_bounds_j) { pathExp->add_rewardBound(parse_RewardBound(reward_bound_j)); }
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return pathExp;
}

std::unique_ptr<QfiedExpression> Parser::parse_QfiedExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    auto qfiedExp = std::make_unique<QfiedExpression>(PARSER::check_op_enum_decoupled<QfiedExpression::Qfier>(input, QfiedExpression::str_to_qfier));

    PARSER::check_contains(input, JANI::EXP);
    qfiedExp->set_path_formula(parse_PropertyExpression(input.at(JANI::EXP), true));

    PARSER::check_for_unexpected_element(input, 2);

    return qfiedExp;
}

std::unique_ptr<RealValueExpression> Parser::parse_RealValueExpression(const nlohmann::json& input) const {

    if constexpr (PARSER::decoupledParsing) {

        if (input.is_number()) { return std::make_unique<RealValueExpression>(RealValueExpression::NONE_C, input); }

        /* Else input must be { ANI::CONSTANT: [CONSTANT NAME] }. */
        PARSER::check_object(input);
        PARSER::check_contains(input, JANI::CONSTANT);

    }

    const nlohmann::json& constant_name_j = input.at(JANI::CONSTANT);
    PARSER::check_string(constant_name_j, input);
    PARSER::throw_parser_exception_if(!stringToConstantName.count(constant_name_j), constant_name_j, input, PARSER::unknown_name);
    PARSER::check_for_unexpected_element(input, 1);
    return std::make_unique<RealValueExpression>(stringToConstantName.at(constant_name_j));
}

std::unique_ptr<UnaryOpExpression> Parser::parse_UnaryOpExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    PARSER::check_object_decoupled(input);

    auto unaryOpExp = std::make_unique<UnaryOpExpression>(PARSER::check_op_enum_decoupled<UnaryOpExpression::UnaryOp>(input, UnaryOpExpression::str_to_unary_op));

    PARSER::check_contains(input, JANI::EXP);
    unaryOpExp->set_operand(parse_Expression(input.at(JANI::EXP)));

    PARSER::check_for_unexpected_element(input, 2);

    return unaryOpExp;
}

std::unique_ptr<VariableExpression> Parser::parse_VariableExpression(const nlohmann::json& input) {

    if constexpr (PARSER::decoupledParsing) { PARSER::check_string(input); }

    if (globalVars.count(input)) { return std::make_unique<VariableExpression>(*globalVars.at(input)); }

    if (localVars.count(input)) { return std::make_unique<VariableExpression>(*localVars.at(input)); }

    PARSER::throw_parser_exception(input, PARSER::unknown_name);
}

#pragma GCC diagnostic push
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "UnusedParameter"

std::unique_ptr<BoolValueExpression> Parser::parse_BooleanValueExpression(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::throw_parser_exception_if(not input.is_boolean(), input, PARSER::unmatched_jani_structure);
    return std::make_unique<BoolValueExpression>(input);
}

std::unique_ptr<IntegerValueExpression> Parser::parse_IntegerValueExpression(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::throw_parser_exception_if(not input.is_number_integer(), input, PARSER::unmatched_jani_structure);
    return std::make_unique<IntegerValueExpression>(input);
}

[[maybe_unused]] std::unique_ptr<StatePredicateExpression> Parser::parse_StatePredicateExpression(const nlohmann::json& input) {
    PLAJA_ABORT_IF_CONST(not PARSER::decoupledParsing)
    PARSER::check_object(input);
    PARSER::check_contains(input, JANI::OP);
    const auto& op_j = input.at(JANI::OP);
    PARSER::check_string(op_j, input);
    const auto op = StatePredicateExpression::str_to_op(op_j);
    PARSER::throw_parser_exception_if((input.size() > 1 or not op), op_j, input, PARSER::unmatched_jani_structure);
    return std::make_unique<StatePredicateExpression>(*op);
}

#pragma GCC diagnostic pop

// non-standard:

std::unique_ptr<ActionOpTuple> Parser::parse_action_op_tuple(const nlohmann::json& input) const {
    PARSER::check_object_decoupled(input);
    PARSER::check_for_unexpected_element(input, 2);
    return std::make_unique<ActionOpTuple>(PARSER::parse_integer(input, JANI::ID), PARSER::parse_integer(input, JANI::INDEX));
}

std::unique_ptr<ConstantArrayAccessExpression> Parser::parse_constant_array_access_expression_if(const nlohmann::json& input) {
    PARSER::check_object_decoupled(input);

    /* op */
    PARSER::check_op_decoupled(input, JANI::AA);

    std::unique_ptr<ConstantArrayAccessExpression> const_aa_exp(new ConstantArrayAccessExpression(nullptr));
    /* exp */
    PARSER::check_contains(input, JANI::EXP);
    /* Current support. */
    const nlohmann::json& exp_j = input.at(JANI::EXP);
    if (exp_j.is_string()) {
        auto it = constants.find(exp_j);
        if (it == constants.end()) { return nullptr; }
        const_aa_exp->set_accessed_array(it->second);
    } else {
        return nullptr;
    }

    /* index */
    PARSER::check_contains(input, JANI::INDEX);
    const_aa_exp->set_index(parse_Expression(input.at(JANI::INDEX)));

    PARSER::check_for_unexpected_element(input, 3);
    return const_aa_exp;
}

std::unique_ptr<LetExpression> Parser::parse_LetExpression(const nlohmann::json& input) { // NOLINT(misc-no-recursion)
    // op:
    PARSER::check_object(input);
    PARSER::check_op(input, LetExpression::get_op_string());

    std::unique_ptr<LetExpression> let_exp(new LetExpression());
    unsigned int object_size = 1;

    // free variables
    if (input.contains(JANI::VARIABLES)) {
        ++object_size;
        const nlohmann::json& variables_input = input.at(JANI::VARIABLES);
        PARSER::check_array(variables_input, input);
        let_exp->reserve(variables_input.size());
        for (const auto& free_var_input: variables_input) {
            let_exp->add_free_variable(parse_FreeVariableDeclaration(free_var_input));
        }
    }

    // push free variables
    for (auto it = let_exp->init_free_variable_iterator(); !it.end(); ++it) { freeVars.emplace(it->get_name(), it()); }
    // expression
    PARSER::check_contains(input, JANI::EXPRESSION);
    ++object_size;
    let_exp->set_expression(parse_Expression(input.at(JANI::EXPRESSION)));
    // pop free variables
    for (auto it = let_exp->init_free_variable_iterator(); !it.end(); ++it) { freeVars.erase(it->get_name()); }

    PARSER::check_for_unexpected_element(input, object_size);
    return let_exp;
}

std::unique_ptr<LocationValueExpression> Parser::parse_location_value_expression(const nlohmann::json& input) {
    PARSER::check_object(input);

    // automaton
    // name
    PARSER::check_contains(input, JANI::AUTOMATON);
    const nlohmann::json& automaton_name = input.at(JANI::AUTOMATON);
    PARSER::check_string(automaton_name, input);
    // index
    PARSER::check_contains(input, JANI::INDEX);
    const nlohmann::json& automaton_index = input.at(JANI::INDEX);
    // check automaton index valid
    PARSER::throw_parser_exception_if(not automaton_index.is_number_integer() or automaton_index < 0 or model_ref->get_number_automataInstances() <= automaton_index, automaton_index, input, PARSER::invalid);
    // check name matches index:
    const Automaton* automaton = model_ref->get_automatonInstance(automaton_index);
    PLAJA_ASSERT(automaton)
    PARSER::throw_parser_exception_if(automaton->get_name() != automaton_name, input, input, PARSER::invalid);

    // location
    PARSER::check_contains(input, JANI::LOCATION);
    const nlohmann::json& location_name = input.at(JANI::LOCATION);
    PARSER::check_string(location_name, input);
    // check location exists:
    const Location* location = automaton->get_location_by_name(location_name);
    PARSER::throw_parser_exception_if(not location, location_name, input, PARSER::unknown_name);

    PARSER::check_for_unexpected_element(input, 3);

    return std::make_unique<LocationValueExpression>(*automaton, location);
}

std::unique_ptr<ObjectiveExpression> Parser::parse_ObjectiveExpression(const nlohmann::json& input) {

    // op:
    PARSER::check_object(input);
    PARSER::check_op(input, ObjectiveExpression::get_op_string());

    std::unique_ptr<ObjectiveExpression> obj_exp(new ObjectiveExpression());
    unsigned int object_size = 1;

    // goal:
    if (input.contains(JANI::GOAL)) {
        ++object_size;
        obj_exp->set_goal(parse_Expression(input.at(JANI::GOAL)));
    }
    // goal reward:
    if (input.contains(JANI::GOAL_POTENTIAL)) {
        ++object_size;
        PARSER::allow_let = true;
        obj_exp->set_goal_potential(parse_Expression(input.at(JANI::GOAL_POTENTIAL)));
        PARSER::allow_let = false;
    } else if (input.contains(JANI::GOAL_REWARD)) {
        PLAJA_LOG(PLAJA_UTILS::to_red_string(PLAJA_UTILS::string_f("Warning: using deprecated \"%s\". Use \"%s\" instead.", JANI::GOAL_REWARD.c_str(), JANI::GOAL_POTENTIAL.c_str())));
        ++object_size;
        obj_exp->set_goal_potential(parse_Expression(input.at(JANI::GOAL_REWARD)));
    }
    // step reward:
    if (input.contains(JANI::STEP_REWARD)) {
        ++object_size;
        obj_exp->set_step_reward(parse_Expression(input.at(JANI::STEP_REWARD)));
        // accumulate
        PARSER::check_contains(input, JANI::ACCUMULATE);
        ++object_size;
        obj_exp->set_reward_accumulation(parse_reward_accumulation(input.at(JANI::ACCUMULATE)));
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return obj_exp;
}

std::unique_ptr<PAExpression> Parser::parse_PAExpression(const nlohmann::json& input) {
    PARSER::check_object_decoupled(input);
    PARSER::check_op_decoupled(input, PAExpression::get_op_string());

    std::unique_ptr<PAExpression> paExp(new PAExpression());
    unsigned int object_size = 1;

    // start
    if (input.contains(JANI::START)) {
        PARSER::allow_let = true;
        ++object_size;

        const auto& start_input = load_label_or_external_if(input.at(JANI::START));
        PARSER::check_object(start_input, input);

        /* For parsing (and semantics checks) we still expect StateConditionExpression or StatesValuesExpression. */
        if (start_input.contains(JANI::OP)) {
            const nlohmann::json& op_start = start_input.at(JANI::OP);
            if (op_start == StateConditionExpression::get_op_string()) { paExp->set_start(parse_StateConditionExpression(start_input)); }
            else if (op_start == StatesValuesExpression::get_op_string()) { paExp->set_start(parse_StatesValuesExpression(start_input)); }
            else { PARSER::throw_parser_exception(start_input, input, PARSER::unmatched_jani_structure); }
        } else { PARSER::throw_parser_exception(start_input, input, PARSER::unmatched_jani_structure); }

        PARSER::allow_let = false;
    }

    // reach
    if (input.contains(JANI::REACH)) {
        ++object_size;
        paExp->set_reach(parse_Expression(load_label_or_external_if(input.at(JANI::REACH))));
    }

    // objective:
    if (input.contains(JANI::OBJECTIVE)) {
        ++object_size;
        paExp->set_objective(parse_ObjectiveExpression(load_label_or_external_if(input.at(JANI::OBJECTIVE))));
    }

    // predicates:
    if (input.contains(JANI::PREDICATES)) {
        ++object_size;
        paExp->set_predicates(parse_PredicatesExpression(load_label_or_external_if(input.at(JANI::PREDICATES))));
    }

    // NN file:
    if (input.contains(JANI::FILE)) {
        ++object_size;
        const nlohmann::json& file = input.at(JANI::FILE);
        PARSER::check_string(file, input);
        PLAJA_ASSERT(PLAJA_UTILS::is_relative_path(file))
        paExp->set_nnFile(current_directory_path + static_cast<const std::string&>(file));
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return paExp;
}

std::unique_ptr<PredicatesExpression> Parser::parse_PredicatesExpression(const nlohmann::json& input) {
    std::unique_ptr<PredicatesExpression> predicates(new PredicatesExpression());
    PARSER::check_array(input);
    predicates->reserve(input.size());
    for (const auto& predicate_j: input) { predicates->add_predicate(parse_Expression(predicate_j)); }
    return predicates;
}

std::unique_ptr<ProblemInstanceExpression> Parser::parse_problem_instance_expression(const nlohmann::json& input) {

    PARSER::check_object_decoupled(input);
    PARSER::check_op_decoupled(input, ProblemInstanceExpression::get_op_string());

    std::unique_ptr<ProblemInstanceExpression> problem_instance(new ProblemInstanceExpression());
    unsigned int object_size = 3;

    problem_instance->set_target_step(PARSER::parse_integer(input, JANI::step));
    problem_instance->set_policy_target_step(PARSER::parse_integer(input, JANI::POLICY));

    if (input.contains(JANI::ops)) {
        ++object_size;
        const auto& ops_input = input.at(JANI::ops);
        PARSER::check_array(ops_input, input);
        // const auto ops_size = ops_input.size();
        problem_instance->reserve_op_path();
        for (const auto& op_j: ops_input) { problem_instance->add_op_path_step(parse_action_op_tuple(op_j)); }
    }

    if (input.contains(JANI::START)) {
        ++object_size;
        problem_instance->set_start(parse_Expression(input.at(JANI::START)));
    }

    if (input.contains(JANI::REACH)) {
        ++object_size;
        problem_instance->set_reach(parse_Expression(input.at(JANI::REACH)));
    }

    if (input.contains(JANI::INITIAL_VALUE)) {
        ++object_size;
        problem_instance->set_includes_init(PARSER::parse_bool(input, JANI::INITIAL_VALUE));
    }

    if (input.contains(JANI::PREDICATES)) {
        ++object_size;
        const nlohmann::json* predicates_j = &input.at(JANI::PREDICATES);
        problem_instance->set_predicates(parse_PredicatesExpression(*predicates_j));

        if (input.contains(JANI::paStates)) {
            ++object_size;
            const auto& pa_states_input = input.at(JANI::paStates);
            PARSER::check_array(pa_states_input, input);
            problem_instance->reserve_pa_path();
            for (const auto& pa_state_j: pa_states_input) { problem_instance->add_pa_state_path_step(parse_array_value_expression(pa_state_j)); }
        }

    }

    PARSER::check_for_unexpected_element(input, object_size);

    return problem_instance;
}

std::unique_ptr<StateConditionExpression> Parser::parse_StateConditionExpression(const nlohmann::json& input) {

    // op:
    PARSER::check_object(input);
    PARSER::check_op(input, StateConditionExpression::get_op_string());

    std::unique_ptr<StateConditionExpression> state_condition_exp(new StateConditionExpression());
    unsigned int object_size = 1;

    // location values
    if (input.contains(JANI::LOCATIONS)) {
        ++object_size;
        const nlohmann::json& locations_input = input.at(JANI::LOCATIONS);
        PARSER::check_array(locations_input, input);
        state_condition_exp->reserve(locations_input.size());
        for (const auto& location_value_input: locations_input) {
            state_condition_exp->add_loc_value(parse_location_value_expression(location_value_input));
        }
    }

    // state variable constraint
    if (input.contains(JANI::EXP)) {
        ++object_size;
        state_condition_exp->set_constraint(parse_Expression(input.at(JANI::EXP)));
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return state_condition_exp;
}

std::unique_ptr<StateValuesExpression> Parser::parse_StateValuesExpression(const nlohmann::json& input) {
    // op:
    PARSER::check_object(input);
    // PARSER::check_op(input, STATE_VALUES_EXPRESSION::opString);

    std::unique_ptr<StateValuesExpression> stateValuesExp(new StateValuesExpression());
    // unsigned int object_size = 1;
    unsigned int object_size = 0;

    // location values
    if (input.contains(JANI::LOCATIONS)) {
        ++object_size;
        const nlohmann::json& locations_input = input.at(JANI::LOCATIONS);
        PARSER::check_array(locations_input, input);
        stateValuesExp->reserve(locations_input.size(), 0);
        for (const auto& location_value_input: locations_input) {
            stateValuesExp->add_loc_value(parse_location_value_expression(location_value_input));
        }
    }

    // variable values
    if (input.contains(JANI::VARIABLES)) {
        ++object_size;
        const nlohmann::json& variables_input = input.at(JANI::VARIABLES);
        PARSER::check_array(variables_input, input);
        stateValuesExp->reserve(0, variables_input.size());
        for (const auto& variable_value_input: variables_input) {
            stateValuesExp->add_var_value(parse_VariableValueExpression(variable_value_input));
        }
    }

    PARSER::check_for_unexpected_element(input, object_size);
    return stateValuesExp;
}

std::unique_ptr<StatesValuesExpression> Parser::parse_StatesValuesExpression(const nlohmann::json& input) {

    PARSER::check_object_decoupled(input);
    PARSER::check_op_decoupled(input, StatesValuesExpression::get_op_string());

    std::unique_ptr<StatesValuesExpression> states_values_exp(new StatesValuesExpression());

    PARSER::check_contains(input, JANI::VALUES);
    const nlohmann::json& states_values_input = input.at(JANI::VALUES);
    PARSER::check_array(states_values_input, input);
    states_values_exp->reserve(states_values_input.size());
    for (const auto& state_values_input: states_values_input) {
        states_values_exp->add_state_values(parse_StateValuesExpression(state_values_input));
    }

    PARSER::check_for_unexpected_element(input, 2);
    return states_values_exp;

}

std::unique_ptr<VariableValueExpression> Parser::parse_VariableValueExpression(const nlohmann::json& input) {
    PARSER::check_object(input);

    std::unique_ptr<VariableValueExpression> varValExp(new VariableValueExpression());
    PARSER::check_contains(input, JANI::VAR);
    varValExp->set_var(parse_VariableExpression(input.at(JANI::VAR)));
    PARSER::check_contains(input, JANI::VALUE);
    varValExp->set_val(parse_Expression(input.at(JANI::VALUE)));

    PARSER::check_for_unexpected_element(input, 2);
    return varValExp;
}
