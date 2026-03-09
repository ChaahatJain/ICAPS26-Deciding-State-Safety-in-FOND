//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_COMMENT_EXPRESSION_H
#define PLAJA_COMMENT_EXPRESSION_H

#include <string>
#include "../expression.h"

/**
 * Auxiliary class to customize output.
 * This class is not parseable.
 */
class CommentExpression final: public Expression {

private:
    std::string comment;

public:
    explicit CommentExpression(std::string comment);
    ~CommentExpression() override;
    DELETE_CONSTRUCTOR(CommentExpression)

    /* Construction. */
    inline void set(std::string str) { comment = std::move(str); }

    inline void append(const std::string& str) { comment += str; }

    /* Getter. */
    [[nodiscard]] inline const std::string& get_comment() const { return comment; }

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;

    [[nodiscard]] std::unique_ptr<CommentExpression> deep_copy() const;

};

#endif //PLAJA_COMMENT_EXPRESSION_H
