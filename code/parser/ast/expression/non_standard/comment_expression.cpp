//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "comment_expression.h"
#include "../../../visitor/ast_visitor_const.h"

CommentExpression::CommentExpression(std::string comment_r):
    comment(std::move(comment_r)) {
}

CommentExpression::~CommentExpression() = default;

/* Override. */

void CommentExpression::accept(AstVisitor*) { PLAJA_ABORT }

void CommentExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> CommentExpression::deepCopy_Exp() const { return deep_copy(); }

/**/

std::unique_ptr<CommentExpression> CommentExpression::deep_copy() const { return std::make_unique<CommentExpression>(comment); }
