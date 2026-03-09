//
// This file is part of the PlaJA code base (2019 - 2020).
//

#ifndef PLAJA_INPUT_FEATURE_TO_JANI_H
#define PLAJA_INPUT_FEATURE_TO_JANI_H

#include <memory>
#include "../../parser/using_parser.h"
#include "../../utils/default_constructors.h"

class LValueExpression;

struct InputFeatureToJANI {
public:
    const VariableIndex_type stateIndex; // for arrays the variable index of the array index not the "variable base"

    explicit InputFeatureToJANI(VariableIndex_type state_index);
    virtual ~InputFeatureToJANI() = 0;
    DELETE_CONSTRUCTOR(InputFeatureToJANI)

    virtual std::unique_ptr<InputFeatureToJANI> deep_copy() const = 0;
    [[nodiscard]] virtual const LValueExpression* get_expression() const;

    [[nodiscard]] virtual bool is_location() const;
    [[nodiscard]] virtual bool is_value_variable() const;
    [[nodiscard]] virtual bool is_array_variable() const;

    [[nodiscard]] virtual VariableID_type get_variable_id() const;
    [[nodiscard]] virtual VariableIndex_type get_array_index() const;
};

#endif //PLAJA_INPUT_FEATURE_TO_JANI_H
