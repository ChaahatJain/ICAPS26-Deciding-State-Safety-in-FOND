//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PRECEDENCE_CHECKER_H
#define PLAJA_PRECEDENCE_CHECKER_H

#include "../../ast_forward_declarations.h"
#include "../../ast/expression/binary_op_expression.h"
#include "../../ast/expression/unary_op_expression.h"

/**
 * Auxiliary functions to simplify linear string representation of AST.
 */

namespace PLAJA_TO_STR {

    [[nodiscard]] extern bool is_atomic(const Expression& expr);
    [[nodiscard]] extern bool takes_precedence(const Expression& expr, BinaryOpExpression::BinaryOp parent_op, bool unordered_junctions);
    [[nodiscard]] extern bool takes_precedence(const Expression& expr, UnaryOpExpression::UnaryOp parent_op);

}

#endif //PLAJA_PRECEDENCE_CHECKER_H
