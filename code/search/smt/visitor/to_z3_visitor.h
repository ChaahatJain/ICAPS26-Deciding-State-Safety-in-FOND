//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_Z3_VISITOR_H
#define PLAJA_TO_Z3_VISITOR_H

#include <unordered_map>
#include "../../../parser/visitor/ast_visitor_const.h"
#include "../../../parser/using_parser.h"
#include "../utils/to_z3_expr.h"
#include "../forward_smt_z3.h"

// This visitor itself generates *one* Z3 expression from *one* JANI expression.
class ToZ3Visitor: public AstVisitorConst {

    using FreeVariablesInZ3 = std::unordered_map<FreeVariableID_type, z3::expr>;

private:
    Z3_IN_PLAJA::Context* context;
    const Z3_IN_PLAJA::StateVarsInZ3* varToZ3;
    /* */
    ToZ3Expr rltExpr;
    FreeVariablesInZ3 freeVars;

    void visit(const FreeVariableDeclaration* var_decl) override;

    void visit(const ArrayAccessExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const FreeVariableExpression* exp) override;
    void visit(const ITE_Expression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    /* Non-standard. */
    void visit(const ConstantArrayAccessExpression* exp) override;
    void visit(const LetExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const StateValuesExpression* exp) override;
    void visit(const StatesValuesExpression* exp) override;
    /* Special case. */
    void visit(const NaryExpression* exp) override;
    void visit(const LinearExpression* exp) override;

    void visit(const BoundedType* type) override;

    explicit ToZ3Visitor(const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

    ToZ3Expr to_z3(const Expression& exp);
    z3::expr to_z3_condition(const Expression& exp);
public:
    ~ToZ3Visitor() override;
    DELETE_CONSTRUCTOR(ToZ3Visitor)

    /**
     * Compute a Z3 expression of a JANI expression.
     * @param exp the JANI expression to generate a Z3 expression for.
     * @param var_to_z3 the variable ID to z3 expression mapping to be used, i.e., expression of constants of the Z3's context.
     * @return a pair of the primary Z3 expression and an auxiliary Z3 expression, which is bool or empty.
     * Instances where auxiliary expression are needed: for arrays we need to bound the index expression, for rounding we introduce auxiliary variables which must be bounded too.
     */
    static ToZ3Expr to_z3(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

    /**
     * Compute a Z3 expression of a JANI expression.
     * If there is an auxiliary expression return the conjunction of main and auxiliary expression
     * @param exp an expression of type bool
     */
    static z3::expr to_z3_condition(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

    static std::unique_ptr<ToZ3ExprSplits> to_z3_condition_splits(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

};

namespace Z3_IN_PLAJA {

    extern ToZ3Expr to_z3(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);
    extern z3::expr to_z3_condition(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);
    extern std::unique_ptr<ToZ3ExprSplits> to_z3_condition_splits(const Expression& exp, const Z3_IN_PLAJA::StateVarsInZ3& var_to_z3);

    extern z3::expr to_conjunction(const z3::expr_vector& vec);
    extern z3::expr to_conjunction(const std::vector<z3::expr>& vec);
    extern z3::expr to_disjunction(const z3::expr_vector& vec);
    extern z3::expr to_disjunction(const std::vector<z3::expr>& vec);
}

#endif //PLAJA_TO_Z3_VISITOR_H
