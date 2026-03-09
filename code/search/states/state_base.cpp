//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "state_base.h"
#include "../../exception/runtime_exception.h"
#include "../../parser/ast/expression/expression_utils.h"
#include "../../parser/ast/expression/lvalue_expression.h"
#include "../../parser/ast/automaton.h"
#include "../../parser/ast/model.h"
#include "../../parser/ast/variable_declaration.h"
#include "../../utils/floating_utils.h"
#include "../../utils/utils.h"
#include "../../globals.h"
#include "../information/model_information.h"

FIELD_IF_RUNTIME_CHECKS(bool StateBase::suppressWrittenTo(false);)

FIELD_IF_DEBUG(bool StateBase::suppressIsValid(false);)

StateBase::StateBase(std::size_t PARAM_IF_RUNTIME_CHECKS(state_size)) { STMT_IF_RUNTIME_CHECKS(writtenTo = std::vector<bool>(state_size)) }

StateBase::~StateBase() = default;

StateBase::StateBase(const StateBase& PARAM_IF_RUNTIME_CHECKS(state)) { STMT_IF_RUNTIME_CHECKS(writtenTo = std::vector<bool>(state.size())) }

StateBase& StateBase::operator=(const StateBase& PARAM_IF_RUNTIME_CHECKS(other)) { // NOLINT(modernize-use-equals-default)
    STMT_IF_RUNTIME_CHECKS(writtenTo = std::vector<bool>(other.writtenTo.size(), false))
    return *this;
}

/**********************************************************************************************************************/

#ifdef RUNTIME_CHECKS

void StateBase::update_written_to(VariableIndex_type index) {
    RUNTIME_ASSERT(suppressWrittenTo or not writtenTo[index], "double assignment within one sequence.")
    STMT_IF_RUNTIME_CHECKS(writtenTo[index] = true)
}

#endif

#ifndef NDEBUG

bool StateBase::is_valid(VariableIndex_type state_index, PLAJA::integer value) {
    PLAJA_ASSERT(PLAJA_GLOBAL::currentModel)
    return suppressIsValid or PLAJA_GLOBAL::currentModel->get_model_information().is_valid(state_index, value);
}

bool StateBase::is_valid(VariableIndex_type state_index, PLAJA::floating value) {
    PLAJA_ASSERT(PLAJA_GLOBAL::currentModel)
    return suppressIsValid or PLAJA_GLOBAL::currentModel->get_model_information().is_valid(state_index, value);
}

bool StateBase::is_valid(VariableIndex_type state_index) {
    PLAJA_ASSERT(PLAJA_GLOBAL::currentModel)
    return suppressIsValid or PLAJA_GLOBAL::currentModel->get_model_information().is_valid(*this, state_index);
}

bool StateBase::is_valid() const {
    PLAJA_ASSERT(PLAJA_GLOBAL::currentModel)
    return suppressIsValid or PLAJA_GLOBAL::currentModel->get_model_information().is_valid(*this);
}

#endif

/**********************************************************************************************************************/

std::unique_ptr<class Expression> StateBase::get_value(const class LValueExpression& var) const {
    const auto state_index = var.get_variable_index(this);
    if (state_index < get_int_state_size()) {
        const auto value = get_int(state_index);
        return var.is_boolean() ? PLAJA_EXPRESSION::make_bool(value) : PLAJA_EXPRESSION::make_int(value);
    } else {
        PLAJA_ASSERT(var.is_floating())
        return PLAJA_EXPRESSION::make_real(get_float(state_index));
    }
}

/**********************************************************************************************************************/

bool StateBase::operator==(const StateBase& other) const {
    const auto state_size = size();
    // size:
    if (state_size != other.size()) { return false; }
    // values:
    const auto int_state_size = get_int_state_size();
    for (VariableIndex_type state_index = 0; state_index < int_state_size; ++state_index) { if (this->get_int(state_index) != other.get_int(state_index)) { return false; } }
    const auto floating_state_size = get_floating_state_size();
    for (VariableIndex_type state_index = int_state_size; state_index < floating_state_size; ++state_index) { if (PLAJA_FLOATS::non_equal(this->get_float(state_index), other.get_float(state_index), PLAJA::floatingPrecision)) { return false; } }
    //
    return true;
}

bool StateBase::equal(const StateBase& other, VariableIndex_type state_index) const {
    return state_index < get_int_state_size() ? this->get_int(state_index) == other.get_int(state_index) : PLAJA_FLOATS::equal(this->get_float(state_index), other.get_float(state_index), PLAJA::floatingPrecision);
}

std::size_t StateBase::hash() const {
    constexpr unsigned prime = 31;
    std::size_t result = 1;

    /* Int. */
    VariableIndex_type state_index = 0;
    const auto int_size = get_int_state_size();
    for (; state_index < int_size; ++state_index) { result = prime * result + get_int(state_index); }

    /* Float. */
    for (const auto state_size = int_size + get_floating_state_size(); state_index < state_size; ++state_index) { result = prime * result + PLAJA_UTILS::hashFloating(get_float(state_index)); }

    return result;
}

bool StateBase::lte(const StateBase& other, VariableIndex_type state_index) const {
    return state_index < get_int_state_size() ? this->get_int(state_index) <= other.get_int(state_index) : PLAJA_FLOATS::lte(this->get_float(state_index), other.get_float(state_index), PLAJA::floatingPrecision);
}

bool StateBase::lt(const StateBase& other, VariableIndex_type state_index) const {
    return state_index < get_int_state_size() ? this->get_int(state_index) < other.get_int(state_index) : PLAJA_FLOATS::lt(this->get_float(state_index), other.get_float(state_index), PLAJA::floatingPrecision);
}

/**********************************************************************************************************************/

#include "../../parser/ast/expression/non_standard/comment_expression.h"
#include "../../parser/ast/expression/non_standard/location_value_expression.h"
#include "../../parser/ast/expression/non_standard/variable_value_expression.h"
#include "../../parser/ast/expression/non_standard/state_condition_expression.h"
#include "../../parser/ast/expression/special_cases/nary_expression.h"
#include "../../parser/ast/expression/special_cases/linear_expression.h"
#include "../../parser/ast/expression/array_value_expression.h"
#include "../../parser/ast/expression/array_access_expression.h"
#include "../../parser/ast/expression/binary_op_expression.h"
#include "../../parser/ast/expression/constant_value_expression.h"
#include "../../parser/ast/expression/unary_op_expression.h"
#include "../../parser/ast/expression/variable_expression.h"

std::unique_ptr<Expression> StateBase::to_condition(bool do_locs, const Model& model) const {

    const auto& model_info = model.get_model_information();

    auto var_condition = std::make_unique<NaryExpression>(BinaryOpExpression::AND);
    var_condition->reserve(size() - (do_locs ? model_info.get_automata_offset() : 0));

    for (auto it = model_info.stateIndexIteratorInt(false); !it.end(); ++it) {
        const auto state_index = it.state_index();
        auto var_expr = model.gen_var_expr(state_index, nullptr);

        if (PLAJA_UTILS::cast_ref<LValueExpression>(*var_expr).is_boolean()) {

            if (get_int(state_index)) { var_condition->add_sub(std::move(var_expr)); }
            else {
                auto not_var = std::make_unique<UnaryOpExpression>(UnaryOpExpression::NOT);
                not_var->set_operand(std::move(var_expr));
                var_condition->add_sub(std::move(not_var));
            }

        } else {
            var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_int(get_int(state_index)), BinaryOpExpression::EQ));
        }

    }

    for (auto it = model_info.stateIndexIteratorFloat(); !it.end(); ++it) {
        const auto state_index = it.state_index();
        var_condition->add_sub(LinearExpression::construct_bound(model.gen_var_expr(state_index, nullptr), PLAJA_EXPRESSION::make_real(get_float(state_index)), BinaryOpExpression::EQ));
        // Negation:
        // var_condition->add_sub(LinearExpression::construct_bound(var_expr->deepCopy_Exp(), PLAJA_EXPRESSION::make_int_expr(std::floor(get_float(state_index))), BinaryOpExpression::GE));
        // var_condition->add_sub(LinearExpression::construct_bound(std::move(var_expr), PLAJA_EXPRESSION::make_int_expr(std::ceil(get_float(state_index))), BinaryOpExpression::LE));
    }

    if (not do_locs) { return var_condition; }

    auto state_condition = std::make_unique<StateConditionExpression>();

    for (auto it = model_info.locationIndexIterator(); !it.end(); ++it) {
        const auto location = PLAJA_UTILS::cast_numeric<AutomatonIndex_type>(it.location_index());
        state_condition->add_loc_value(PLAJA_UTILS::cast_unique<LocationValueExpression>(model.gen_loc_value_expr(location, get_int(location))));
    }

    state_condition->set_constraint(std::move(var_condition));

    return state_condition;
}

std::unique_ptr<Expression> StateBase::loc_to_expr(const Automaton& instance) const {
    return std::make_unique<LocationValueExpression>(instance, instance.get_location(get_int(instance.get_index())));
}

std::unique_ptr<Expression> StateBase::var_to_expr(const VariableDeclaration& var, const std::size_t* offset_index) const {

    auto var_index = var.get_index();
    auto var_size = var.get_size();

    if (offset_index) {
        PLAJA_ASSERT(var.is_array())
        PLAJA_ASSERT(*offset_index < var_size)

        auto rlt = std::make_unique<BinaryOpExpression>(BinaryOpExpression::EQ);

        auto aa = std::make_unique<ArrayAccessExpression>();
        aa->set_accessedArray(var);
        aa->set_index(PLAJA_EXPRESSION::make_int(PLAJA_UTILS::cast_numeric<PLAJA::integer>(*offset_index)));

        rlt->set_left(std::move(aa));
        rlt->set_right(rlt->get_left()->evaluate(this));
        return rlt;
    }

    auto rlt = std::make_unique<VariableValueExpression>();
    rlt->set_var(std::make_unique<VariableExpression>(var));

    if (var_size == 1) {

        PLAJA_ASSERT(var.is_scalar())
        rlt->set_val(rlt->get_var()->evaluate(this));

    } else {
        PLAJA_ASSERT(var.is_array())

        auto values = std::make_unique<ArrayValueExpression>();
        values->reserve(var_size);
        for (std::size_t offset = 0; offset < var_size; ++offset) {
            if (var_index < get_int_state_size()) { values->add_element(PLAJA_EXPRESSION::make_int(get_int(var_index + offset))); }
            else { values->add_element(PLAJA_EXPRESSION::make_real(get_float(var_index + offset))); }
        }
    }

    return rlt;

}

std::unique_ptr<Expression> StateBase::to_array_value(const Model* model) const {

    auto values = std::make_unique<ArrayValueExpression>();
    const auto state_size = this->size();
    values->reserve(state_size);

    if (model) {

        /* Locations. */
        auto automata_offset = model->get_number_automataInstances();
        for (AutomatonIndex_type automaton_index = 0; automaton_index < automata_offset; ++automaton_index) {
            values->add_element(loc_to_expr(*model->get_automatonInstance(automaton_index)));
        }

        // Variables. */
        for (auto var_it = model->variableIterator(); !var_it.end(); ++var_it) {
            values->add_element(var_to_expr(*var_it.variable(), nullptr));
        }

    } else {

        VariableIndex_type state_index = 0;
        for (; state_index < get_int_state_size(); ++state_index) {
            values->add_element(PLAJA_EXPRESSION::make_int(get_int(state_index)));
        }

        for (; state_index < state_size; ++state_index) {
            values->add_element(PLAJA_EXPRESSION::make_real(get_float(state_index)));
        }

    }

    return values;

}

std::unique_ptr<Expression> StateBase::locs_to_array_value(const Model& model) const {
    auto values = std::make_unique<ArrayValueExpression>();
    auto automata_offset = model.get_number_automataInstances();
    values->reserve(automata_offset);
    for (AutomatonIndex_type automaton_index = 0; automaton_index < automata_offset; ++automaton_index) {
        values->add_element(loc_to_expr(*model.get_automatonInstance(automaton_index)));
    }
    return values;
}

/* output */

void StateBase::dump(const Model* model) const { to_array_value(model)->dump(true); }

std::string StateBase::to_str(const Model* model) const { return to_array_value(model)->to_string(true); }
std::string StateBase::save_as_str() const {
    std::string val = "[";
    const auto state_size = this->size();
    for (VariableIndex_type i = 0; i < state_size; ++i) {
        if (i < get_int_state_size()) {
            val += std::to_string(get_int(i)) + "~";
        } else {
            val += std::to_string(get_float(i)) + "~";
        }
    }
    val += "]";
    return val;
}

#ifndef NDEBUG

void StateBase::dump(bool use_global_model) const { dump(use_global_model ? PLAJA_GLOBAL::currentModel : nullptr); }

std::string StateBase::to_str(bool use_global_model) const { return to_str(use_global_model ? PLAJA_GLOBAL::currentModel : nullptr); }

/* Pattern dump. */

std::unique_ptr<Expression> StateBase::to_array_value(const std::vector<std::string>& loc_pattern, const std::vector<std::string>& var_pattern) const {

    const auto& model = *PLAJA_GLOBAL::currentModel;

    auto values = std::make_unique<ArrayValueExpression>();
    values->reserve(loc_pattern.size() + var_pattern.size());

    /* Locations. */
    for (const auto& name: loc_pattern) {
        values->add_element(loc_to_expr(*model.get_automaton_by_name(name)));
    }

    /* Variables. */
    for (const auto& name: var_pattern) {
        values->add_element(var_to_expr(model.get_variable_by_name(name), nullptr));
    }

    return values;
}

void StateBase::dump(const std::vector<std::string>& loc_pattern, const std::vector<std::string>& var_pattern) const {
    to_array_value(loc_pattern, var_pattern)->dump(true);
}

void StateBase::dump(const std::vector<unsigned int>& tab_pattern) const {

    auto values = std::make_unique<ArrayValueExpression>();
    const auto state_size = this->size();
    values->reserve(state_size + tab_pattern.size());

    if (tab_pattern.empty()) {
        dump();
        return;
    }

    auto next_tab = tab_pattern[0];
    auto next_tab_index = 0;

    for (VariableIndex_type i = 0; i < state_size; ++i) {
        auto value = i < get_int_state_size() ? PLAJA_EXPRESSION::make_int(get_int(i)) : PLAJA_EXPRESSION::make_real(get_float(i));

        if (next_tab == i) {

            values->add_element(std::make_unique<CommentExpression>(PLAJA_UTILS::tabString + value->to_string(true)));

            if (++next_tab_index < tab_pattern.size()) { next_tab += tab_pattern[next_tab_index]; }
            else { next_tab = 0; }

        } else { values->add_element(std::move(value)); }

    }

    values->dump(true);
}

/* getter */

PLAJA::integer StateBase::loc_value_by_name(const std::string& var_name) const {
    return this->get_int(PLAJA_GLOBAL::currentModel->get_automaton_by_name(var_name)->get_index());
}

const std::string& StateBase::loc_by_name(const std::string& var_name) const {
    const auto* instance = PLAJA_GLOBAL::currentModel->get_automaton_by_name(var_name);
    return instance->get_location_name(this->get_int(instance->get_index()));
}

PLAJA::floating StateBase::operator[](const std::string& var_name) const {
    try { return this->operator[](PLAJA_GLOBAL::currentModel->get_variable_index_by_name(var_name)); }
    catch (PlajaException& e) { return loc_value_by_name(var_name); }
}

#endif