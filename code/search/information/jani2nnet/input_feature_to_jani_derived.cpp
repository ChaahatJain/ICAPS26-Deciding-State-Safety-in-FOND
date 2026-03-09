//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "input_feature_to_jani_derived.h"
#include "../../../parser/ast/expression/array_access_expression.h"
#include "../../../parser/ast/variable_declaration.h"

InputFeatureToLocation::InputFeatureToLocation(VariableIndex_type state_index):
    InputFeatureToJANI(state_index) {}

std::unique_ptr<InputFeatureToJANI> InputFeatureToLocation::deep_copy() const {
    return std::make_unique<InputFeatureToLocation>(stateIndex);
}

bool InputFeatureToLocation::is_location() const { return true; }

//

InputFeatureToVariable::InputFeatureToVariable(const VariableExpression& var_):
    InputFeatureToJANI(var_.get_index())
    , var(var_.deep_copy()) {}

InputFeatureToVariable::InputFeatureToVariable(const VariableDeclaration& var_decl):
    InputFeatureToJANI(var_decl.get_index())
    , var(new VariableExpression(var_decl)) {}

InputFeatureToVariable::~InputFeatureToVariable() = default;

std::unique_ptr<InputFeatureToJANI> InputFeatureToVariable::deep_copy() const { return std::unique_ptr<InputFeatureToVariable>(new InputFeatureToVariable(*var)); }

const LValueExpression* InputFeatureToVariable::get_expression() const { return var.get(); }

bool InputFeatureToVariable::is_value_variable() const { return true; }

VariableID_type InputFeatureToVariable::get_variable_id() const { return var->get_variable_id(); }

//


InputFeatureToArray::InputFeatureToArray(const ArrayAccessExpression& array_access):
    InputFeatureToJANI(array_access.get_variable_index())
    , arrayAccess(array_access.deepCopy()) {
}

InputFeatureToArray::InputFeatureToArray(const VariableDeclaration& var_decl, PLAJA::integer array_index):
    InputFeatureToJANI(var_decl.get_index() + array_index)
    , arrayAccess(new ArrayAccessExpression()) {
    arrayAccess->set_accessedArray(var_decl);
    arrayAccess->set_index(array_index);
}

InputFeatureToArray::~InputFeatureToArray() = default;

std::unique_ptr<InputFeatureToJANI> InputFeatureToArray::deep_copy() const { return std::unique_ptr<InputFeatureToArray>(new InputFeatureToArray(*arrayAccess)); }

const LValueExpression* InputFeatureToArray::get_expression() const { return arrayAccess.get(); }

bool InputFeatureToArray::is_array_variable() const { return true; }

VariableID_type InputFeatureToArray::get_variable_id() const { return arrayAccess->get_variable_id(); }

VariableIndex_type InputFeatureToArray::get_array_index() const {
    PLAJA_ASSERT(arrayAccess->get_index()->is_constant())
    return arrayAccess->get_variable_index();
}

