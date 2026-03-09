//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_INPUT_FEATURE_TO_JANI_DERIVED_H
#define PLAJA_INPUT_FEATURE_TO_JANI_DERIVED_H

#include "../../../parser/ast/expression/forward_expression.h"
#include "../../../parser/ast/forward_ast.h"
#include "input_feature_to_jani.h"

struct InputFeatureToLocation: public InputFeatureToJANI {

    explicit InputFeatureToLocation(VariableIndex_type state_index);
    ~InputFeatureToLocation() override = default;
    DELETE_CONSTRUCTOR(InputFeatureToLocation)

    std::unique_ptr<InputFeatureToJANI> deep_copy() const override;

    [[nodiscard]] bool is_location() const override;

};

struct InputFeatureToVariable: public InputFeatureToJANI {

protected:
    std::unique_ptr<VariableExpression> var;
    explicit InputFeatureToVariable(const VariableExpression& var_);

public:
    explicit InputFeatureToVariable(const VariableDeclaration& var_decl);
    ~InputFeatureToVariable() override;
    DELETE_CONSTRUCTOR(InputFeatureToVariable)

    std::unique_ptr<InputFeatureToJANI> deep_copy() const override;
    [[nodiscard]] const LValueExpression* get_expression() const override;

    [[nodiscard]] bool is_value_variable() const override;
    [[nodiscard]] VariableID_type get_variable_id() const override;

};

struct InputFeatureToArray: public InputFeatureToJANI {

protected:
    std::unique_ptr<ArrayAccessExpression> arrayAccess;
    explicit InputFeatureToArray(const ArrayAccessExpression& array_access);

public:
    InputFeatureToArray(const VariableDeclaration& var_decl, PLAJA::integer array_index);
    ~InputFeatureToArray() override;
    DELETE_CONSTRUCTOR(InputFeatureToArray)

    std::unique_ptr<InputFeatureToJANI> deep_copy() const override;
    [[nodiscard]] const LValueExpression* get_expression() const override;

    [[nodiscard]] bool is_array_variable() const override;
    [[nodiscard]] VariableID_type get_variable_id() const override;
    [[nodiscard]] VariableIndex_type get_array_index() const override;

};

#endif //PLAJA_INPUT_FEATURE_TO_JANI_DERIVED_H
