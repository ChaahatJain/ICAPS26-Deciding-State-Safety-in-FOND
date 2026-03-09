//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FREE_VARIABLE_EXPRESSION_H
#define PLAJA_FREE_VARIABLE_EXPRESSION_H


#include "expression.h"

class FreeVariableExpression: public Expression {

private:
    std::string name;
    FreeVariableID_type id; // id of underlying free variable

public:
    explicit FreeVariableExpression(std::string name_, FreeVariableID_type var_id, const DeclarationType* type);
    ~FreeVariableExpression() override;

    // setter:
    inline void set_name(const std::string& name_) { name = name_; }
    inline void set_id(FreeVariableID_type var_id) { id = var_id; }

    // getter:
    [[nodiscard]] inline const std::string& get_name() const { return name; }
    [[nodiscard]] inline FreeVariableID_type get_id() const { return id; }

    // override:
    const DeclarationType* determine_type() override;
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
    [[nodiscard]] std::unique_ptr<Expression> deepCopy_Exp() const override;
    //
    [[nodiscard]] std::unique_ptr<FreeVariableExpression> deepCopy() const;
};


#endif //PLAJA_FREE_VARIABLE_EXPRESSION_H
