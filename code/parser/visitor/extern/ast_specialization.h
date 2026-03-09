//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_SPECIALIZATION_H
#define PLAJA_AST_SPECIALIZATION_H

#include "visitor_base.h"

namespace AST_SPECIALIZATION {

    extern void specialize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern std::unique_ptr<AstElement> specialize_ast(const AstElement& ast_elem);
    extern void specialize_ast(class Model& model);

    template<typename AstElement_t>
    inline void specialize_ast(std::unique_ptr<AstElement_t>& ast_elem_) { PLAJA_VISITOR::visit_cast<AstElement_t, AST_SPECIALIZATION::specialize_ast>(ast_elem_); }

}

#endif //PLAJA_AST_SPECIALIZATION_H
