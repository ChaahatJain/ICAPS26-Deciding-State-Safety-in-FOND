//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_STANDARDIZATION_H
#define PLAJA_AST_STANDARDIZATION_H

#include "visitor_base.h"

namespace AST_STANDARDIZATION {

    extern void standardize_ast(std::unique_ptr<AstElement>& ast_elem);
    extern std::unique_ptr<AstElement> standardize_ast(const AstElement& ast_elem);
    extern void standardize_ast(class Model& model);

    template<typename AstElement_t>
    inline void standardize_ast(std::unique_ptr<AstElement_t>& ast_elem) { PLAJA_VISITOR::visit_cast<AstElement_t, AST_STANDARDIZATION::standardize_ast>(ast_elem); }

    template<typename AstElement_t>
    inline std::unique_ptr<AstElement_t> standardize_ast(const AstElement_t& ast_elem) { return PLAJA_VISITOR::visit_cast<AstElement_t, AST_STANDARDIZATION::standardize_ast>(ast_elem); }

}

#endif //PLAJA_AST_STANDARDIZATION_H
