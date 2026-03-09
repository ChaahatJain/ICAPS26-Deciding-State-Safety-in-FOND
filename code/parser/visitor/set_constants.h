//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SET_CONSTANTS_H
#define PLAJA_SET_CONSTANTS_H

#include <unordered_map>
#include "../using_parser.h"
#include "../ast/forward_ast.h"

namespace SET_CONSTANTS {

    extern void set_constants(const std::unordered_map<ConstantIdType, const ConstantDeclaration*>& id_to_const, AstElement& ast_el);

}

#endif //PLAJA_SET_CONSTANTS_H
