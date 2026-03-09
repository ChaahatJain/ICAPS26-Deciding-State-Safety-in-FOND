//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SET_VARIABLE_INDEX_H
#define PLAJA_SET_VARIABLE_INDEX_H

#include <unordered_map>
#include <vector>
#include "../../search/information/forward_information.h"
#include "../using_parser.h"

class AstElement;

class Model;

namespace SET_VARS {

    extern void update_variable_id(const std::unordered_map<VariableID_type, VariableID_type>& old_to_new, AstElement& ast_element);

    extern void set_var_index_by_id(const std::vector<VariableIndex_type>& id_to_index, AstElement& ast_el, const ModelInformation* model_info = nullptr);

    extern void set_var_index_by_id(const Model& model, AstElement& ast_el);

    extern void set_var_index_by_index(const std::vector<int>& index_to_index, const AstElement& ast_el);

}

#endif //PLAJA_SET_VARIABLE_INDEX_H
