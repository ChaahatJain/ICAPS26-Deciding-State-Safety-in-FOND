//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "input_feature_to_jani.h"
#include "../../../exception/not_implemented_exception.h"

InputFeatureToJANI::InputFeatureToJANI(VariableIndex_type state_index): stateIndex(state_index) {}

InputFeatureToJANI::~InputFeatureToJANI() = default;

const LValueExpression* InputFeatureToJANI::get_expression() const { throw NotImplementedException(__PRETTY_FUNCTION__ ); }

bool InputFeatureToJANI::is_location() const { return false; }

bool InputFeatureToJANI::is_value_variable() const { return false; }

bool InputFeatureToJANI::is_array_variable() const { return false; }

VariableID_type InputFeatureToJANI::get_variable_id() const { throw NotImplementedException(__PRETTY_FUNCTION__ ); }

VariableIndex_type InputFeatureToJANI::get_array_index() const { return -1; }
