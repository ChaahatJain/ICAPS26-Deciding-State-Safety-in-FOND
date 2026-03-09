//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include <algorithm>
#include "variables_of.h"
#include "../../utils/utils.h"
#include "../ast/expression/non_standard/constant_array_access_expression.h"
#include "../ast/expression/non_standard/location_value_expression.h"
#include "../ast/expression/non_standard/variable_value_expression.h"
#include "../ast/expression/array_access_expression.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/assignment.h"
#include "../ast/destination.h"
#include "../ast/edge.h"
#include "ast_visitor_const.h"

/**
 * Collect the (constant) variable ids occurring as r-value (and optionally l-value) in the ast element.
 */
class VariablesOf: public AstVisitorConst {

private:
    std::unordered_set<VariableID_type> vars;
    bool ignoreLValues;

    void visit(const Assignment* assignment) override;
    void visit(const VariableExpression* exp) override;

public:
    explicit VariablesOf(bool ignore_lvalues);
    ~VariablesOf() override;
    DELETE_CONSTRUCTOR(VariablesOf)

    [[nodiscard]] inline std::unordered_set<VariableID_type> retrieve() { return std::move(vars); }

};

VariablesOf::VariablesOf(bool ignore_lvalues):
    vars()
    , ignoreLValues(ignore_lvalues) {

}

VariablesOf::~VariablesOf() = default;

/* */

void VariablesOf::visit(const Assignment* assignment) {
    JANI_ASSERT(assignment->get_ref())

    if (ignoreLValues) {
        const auto* array_index = assignment->get_ref()->get_array_index();
        if (array_index) { array_index->accept(this); } // array index is an rValue ...
    } else { assignment->get_ref()->accept(this); }

    if (assignment->is_deterministic()) {
        JANI_ASSERT(assignment->get_value())
        assignment->get_value()->accept(this);
    } else {
        PLAJA_ASSERT(assignment->is_non_deterministic())
        JANI_ASSERT(assignment->get_lower_bound() and assignment->get_upper_bound())
        assignment->get_lower_bound()->accept(this);
        assignment->get_upper_bound()->accept(this);
    }
}

void VariablesOf::visit(const VariableExpression* exp) { vars.insert(exp->get_variable_id()); }

/* */

std::unordered_set<VariableID_type> VARIABLES_OF::vars_of(const AstElement* ast_element, bool ignore_lvalues) {
    PLAJA_ASSERT(ast_element)
    VariablesOf vars_of_visitor(ignore_lvalues);
    ast_element->accept(&vars_of_visitor);
    return vars_of_visitor.retrieve();
}

/**********************************************************************************************************************/

class NonDetUpdatedVarsOf final: public AstVisitorConst {

private:
    std::unordered_set<VariableID_type> vars;

    void visit(const Assignment* assignment) override;

public:
    NonDetUpdatedVarsOf();
    ~NonDetUpdatedVarsOf() override;
    DELETE_CONSTRUCTOR(NonDetUpdatedVarsOf)

    [[nodiscard]] inline std::unordered_set<VariableID_type> retrieve() { return std::move(vars); }

};

NonDetUpdatedVarsOf::NonDetUpdatedVarsOf() = default;

NonDetUpdatedVarsOf::~NonDetUpdatedVarsOf() = default;

void NonDetUpdatedVarsOf::visit(const Assignment* assignment) {
    JANI_ASSERT(assignment->get_ref())
    if (assignment->is_non_deterministic()) { vars.insert(assignment->get_ref()->get_variable_id()); }
}

std::unordered_set<VariableID_type> VARIABLES_OF::non_det_updated_vars_of(const AstElement* ast_element) {
    NonDetUpdatedVarsOf non_det_updated_vars_of;
    ast_element->accept(&non_det_updated_vars_of);
    return non_det_updated_vars_of.retrieve();
}

/**********************************************************************************************************************/

/**
 * Collect the (constant) variable indexes occurring as r-value (and optionally l-value) in the ast element,
 * \ie, the indexes of value variables
 * but also those of array indexes occurring as constant-index accesses,
 * and optionally also location indexes.
 */
class VariableIndexesOf: public AstVisitorConst {

private:
    std::unordered_set<VariableIndex_type> varIndexes;
    bool ignoreLValues;
    bool ignoreLocations;

    void visit(const Assignment* assignment) override;
    void visit(const Destination* dest) override;
    void visit(const Edge* edge) override;
    void visit(const ArrayAccessExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    void visit(const LocationValueExpression* exp) override;

public:
    VariableIndexesOf(bool ignore_lvalues, bool ignore_locations);
    ~VariableIndexesOf() override;
    DELETE_CONSTRUCTOR(VariableIndexesOf)

    [[nodiscard]] inline std::unordered_set<VariableID_type> retrieve() { return std::move(varIndexes); }

};

VariableIndexesOf::VariableIndexesOf(bool ignore_lvalues, bool ignore_locations):
    varIndexes()
    , ignoreLValues(ignore_lvalues)
    , ignoreLocations(ignore_locations) {
}

VariableIndexesOf::~VariableIndexesOf() = default;

/* */

void VariableIndexesOf::visit(const Assignment* assignment) {
    JANI_ASSERT(assignment->get_ref())

    if (ignoreLValues) {
        const auto* array_index = assignment->get_ref()->get_array_index();
        if (array_index) { array_index->accept(this); } // array index is an rValue ...
    } else { assignment->get_ref()->accept(this); }

    if (assignment->is_deterministic()) {
        JANI_ASSERT(assignment->get_value())
        assignment->get_value()->accept(this);
    } else {
        PLAJA_ASSERT(assignment->is_non_deterministic())
        JANI_ASSERT(assignment->get_lower_bound() and assignment->get_upper_bound())
        assignment->get_lower_bound()->accept(this);
        assignment->get_upper_bound()->accept(this);
    }
}

void VariableIndexesOf::visit(const Destination* dest) {
    AstVisitorConst::visit(dest);
    if (not ignoreLocations) { varIndexes.insert(dest->get_location_index()); }
}

void VariableIndexesOf::visit(const Edge* edge) {
    AstVisitorConst::visit(edge);
    if (not ignoreLocations) { varIndexes.insert(edge->get_location_index()); }
}

void VariableIndexesOf::visit(const ArrayAccessExpression* exp) {
    PLAJA_ASSERT(exp->get_index()->is_constant()) // else: // TODO need to add all state indexes -> need model info
    varIndexes.insert(exp->get_variable_index());
}

void VariableIndexesOf::visit(const VariableExpression* exp) { varIndexes.insert(exp->get_variable_index()); }

/* Non-standard. */

void VariableIndexesOf::visit(const LocationValueExpression* exp) { if (not ignoreLocations) { varIndexes.insert(exp->get_location_index()); } }

/* */

std::unordered_set<VariableID_type> VARIABLES_OF::state_indexes_of(const AstElement* ast_element, bool ignore_lvalues, bool ignore_locations) {
    VariableIndexesOf var_indexes_of_visitor(ignore_lvalues, ignore_locations);
    ast_element->accept(&var_indexes_of_visitor);
    return var_indexes_of_visitor.retrieve();
}

/**********************************************************************************************************************/

class ConstantsOf final: public AstVisitorConst {

private:
    bool ignoreInlined;
    std::unordered_set<VariableID_type> constants;

    void visit(const ConstantDeclaration* decl) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const ConstantArrayAccessExpression* exp) override;

public:
    explicit ConstantsOf(bool ignore_inlined);
    ~ConstantsOf() override;
    DELETE_CONSTRUCTOR(ConstantsOf)

    [[nodiscard]] inline std::unordered_set<VariableID_type> retrieve() { return std::move(constants); }

};

ConstantsOf::ConstantsOf(bool ignore_inlined):
    ignoreInlined(ignore_inlined) {}

ConstantsOf::~ConstantsOf() = default;

/* */

void ConstantsOf::visit(const ConstantDeclaration*) { PLAJA_ABORT } // Probably makes no sense to call visitor on a structure that subsumes declarations.

void ConstantsOf::visit(const ConstantExpression* exp) { if (not ignoreInlined) { constants.insert(exp->get_id()); } }

void ConstantsOf::visit(const ConstantArrayAccessExpression* exp) { constants.insert(exp->get_id()); }

/* */

std::unordered_set<VariableID_type> VARIABLES_OF::constants_of(const AstElement& ast_element, bool ignore_inlined) {
    PLAJA_ASSERT_EXPECT(ignore_inlined) // Just because it is currently the only intended usage.
    ConstantsOf constants_of(ignore_inlined);
    ast_element.accept(&constants_of);
    return constants_of.retrieve();
}

/*********************************************************************************************************************/


namespace VARIABLES_OF {

    std::unordered_set<VariableID_type> vars_of(const Expression* expression, bool ignore_lvalues) {
        return VARIABLES_OF::vars_of(PLAJA_UTILS::cast_ptr<AstElement>(expression), ignore_lvalues);
    }

    std::list<VariableID_type> list_vars_of(const AstElement* ast_element, bool ignore_lvalues) {
        auto vars = vars_of(ast_element, ignore_lvalues);
        return { vars.cbegin(), vars.cend() };
    }

    std::unordered_set<VariableIndex_type> state_indexes_of(const Expression* expression, bool ignore_lvalues, bool ignore_locations) {
        return VARIABLES_OF::state_indexes_of(PLAJA_UTILS::cast_ptr<AstElement>(expression), ignore_lvalues, ignore_locations);
    }

    bool contains(const AstElement* ast_element, const std::unordered_set<VariableID_type>& var_set, bool ignore_lvalues) {
        auto vars = VARIABLES_OF::vars_of(ast_element, ignore_lvalues);
        return std::any_of(var_set.cbegin(), var_set.cend(), [vars](VariableID_type var_id) { return vars.count(var_id); });
    }

    bool contains(const AstElement* ast_element, VariableID_type var_id, bool ignore_lvalues) { return VARIABLES_OF::vars_of(ast_element, ignore_lvalues).count(var_id); }

    bool contains_state_indexes(const AstElement* ast_element, const std::unordered_set<VariableIndex_type>& state_indexes_set, bool ignore_lvalues, bool ignore_locations) {
        auto state_indexes = VARIABLES_OF::state_indexes_of(ast_element, ignore_lvalues, ignore_locations);
        return std::any_of(state_indexes_set.cbegin(), state_indexes_set.cend(), [state_indexes](VariableIndex_type state_index) { return state_indexes.count(state_index); });
    }

    bool contains_state_indexes(const AstElement* ast_element, VariableIndex_type state_index, bool ignore_lvalues, bool ignore_locations) { return VARIABLES_OF::state_indexes_of(ast_element, ignore_lvalues, ignore_locations).count(state_index); }

}