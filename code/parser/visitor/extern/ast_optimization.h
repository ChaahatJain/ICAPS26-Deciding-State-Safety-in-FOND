//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_OPTIMIZATION_H
#define PLAJA_AST_OPTIMIZATION_H

#include "visitor_base.h"

namespace AST_OPTIMIZATION {

    extern void optimize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern void optimize_ast(std::unique_ptr<Expression>& expr);
    extern void optimize_ast(Model& model);

    template<typename AstElement_t>
    inline void optimize_ast(std::unique_ptr<AstElement_t>& ast_elem_) { PLAJA_VISITOR::visit_cast<AstElement_t, AST_OPTIMIZATION::optimize_ast>(ast_elem_); }

}

#endif //PLAJA_AST_OPTIMIZATION_H
