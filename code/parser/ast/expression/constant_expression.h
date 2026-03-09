//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTANT_EXPRESSION_H
#define PLAJA_CONSTANT_EXPRESSION_H

#include <memory>
#include "../constant_declaration.h"
#include "expression.h"

/** Reference to a constant declaration. */
class ConstantExpression: public Expression {

private:
    const ConstantDeclaration* decl;

public:
    explicit ConstantExpression(const ConstantDeclaration& decl);
    ~ConstantExpression() override;
    DELETE_CONSTRUCTOR(ConstantExpression)

    /* Setter. */

    void set_constant(const ConstantDeclaration& const_decl);

    /* Getter. */

    [[nodiscard]] const ConstantDeclaration* get_decl() const { return decl; }

    [[nodiscard]] inline const std::string& get_name() const {
        PLAJA_ASSERT(decl)
        return decl->get_name();
    }

    [[nodiscard]] inline ConstantIdType get_id() const {
        PLAJA_ASSERT(decl)
        return decl->get_id();
    }

    [[nodiscard]] inline const Expression* get_value() const {
        PLAJA_ASSERT(decl)
        return decl->get_value();
    }

    /* Override. */
    PLAJA::integer evaluateInteger(const StateBase* state) const override;
    PLAJA::floating evaluateFloating(const StateBase* state) const override;
    [[nodiscard]] bool is_constant() const override;
    bool equals(const Expression* exp) const override;
    [[nodiscard]] std::size_t hash() const override;
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
};

#endif //PLAJA_CONSTANT_EXPRESSION_H
