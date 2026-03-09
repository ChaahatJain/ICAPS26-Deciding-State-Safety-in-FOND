//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VARIABLES_OF_H
#define PLAJA_VARIABLES_OF_H

#include <list>
#include <unordered_set>
#include "../../search/states/forward_states.h"
#include "../ast/expression/forward_expression.h"
#include "../using_parser.h"

class AstElement;

namespace VARIABLES_OF {

    extern std::unordered_set<VariableID_type> vars_of(const AstElement* ast_element, bool ignore_lvalues);
    extern std::unordered_set<VariableID_type> vars_of(const Expression* expression, bool ignore_lvalues); // auxiliary
    extern std::list<VariableID_type> list_vars_of(const AstElement* ast_element, bool ignore_lvalues);

    extern std::unordered_set<VariableID_type> non_det_updated_vars_of(const AstElement* ast_element);

    /** May only be called on structures including constant array-accesses. */
    extern std::unordered_set<VariableIndex_type> state_indexes_of(const AstElement* ast_element, bool ignore_lvalues, bool ignore_locations);
    extern std::unordered_set<VariableIndex_type> state_indexes_of(const Expression* expression, bool ignore_lvalues, bool ignore_locations);

    extern std::unordered_set<ConstantIdType> constants_of(const AstElement& ast_element, bool ignore_inlined);

    extern bool contains(const AstElement* ast_element, const std::unordered_set<VariableID_type>& var_set, bool ignore_lvalues);
    extern bool contains(const AstElement* ast_element, VariableID_type var_id, bool ignore_lvalues);

    extern bool contains_state_indexes(const AstElement* ast_element, const std::unordered_set<VariableIndex_type>& state_indexes_set, bool ignore_lvalues, bool ignore_locations);
    extern bool contains_state_indexes(const AstElement* ast_element, VariableIndex_type state_index, bool ignore_lvalues, bool ignore_locations);

}

#endif //PLAJA_VARIABLES_OF_H
