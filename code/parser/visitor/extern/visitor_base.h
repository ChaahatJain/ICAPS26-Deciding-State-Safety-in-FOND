//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_VISITOR_BASE_H
#define PLAJA_VISITOR_BASE_H

#include <memory>
#include "../../../utils/utils.h"

class AstElement;

class Expression;

namespace PLAJA_VISITOR {

    template<typename AstElement_t, void visit(std::unique_ptr<AstElement>&)>
    inline void visit_cast(std::unique_ptr<AstElement_t>& ast_elem) {
        std::unique_ptr<AstElement> ast_elem_cast = PLAJA_UTILS::cast_unique<AstElement>(std::move(ast_elem));
        visit(ast_elem_cast);
        ast_elem = PLAJA_UTILS::cast_unique<AstElement_t>(std::move(ast_elem_cast));
    }

    template<typename AstElement_t, std::unique_ptr<AstElement> visit(const AstElement&)>
    [[nodiscard]] inline std::unique_ptr<AstElement_t> visit_cast(const AstElement_t& ast_elem) {
        return PLAJA_UTILS::cast_unique<AstElement_t>(visit(ast_elem));
    }

    template<typename AstElement_t, void visit(std::unique_ptr<AstElement_t>&)>
    [[nodiscard]] inline std::unique_ptr<AstElement_t> visit_copy(const AstElement_t& ast_elem) {
        std::unique_ptr<AstElement_t> ast_elem_copy = PLAJA_UTILS::cast_unique<AstElement_t>(ast_elem.deep_copy_ast());
        visit(ast_elem_copy);
        return ast_elem_copy;
    }

    template<typename AstElement_t, void visit(std::unique_ptr<AstElement>&)>
    [[nodiscard]] inline std::unique_ptr<AstElement_t> visit_copy_cast(const AstElement_t& ast_elem) {
        std::unique_ptr<AstElement> ast_elem_copy = ast_elem.deep_copy_ast();
        visit(ast_elem_copy);
        return PLAJA_UTILS::cast_unique<AstElement_t>(std::move(ast_elem_copy));
    }

}

#endif //PLAJA_VISITOR_BASE_H
