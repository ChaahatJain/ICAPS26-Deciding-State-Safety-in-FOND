//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_VERITAS_VISITOR_H
#define PLAJA_TO_VERITAS_VISITOR_H

#include <memory>
#include "../predicates/Equation.h"
#include "../../../parser/visitor/ast_visitor_const.h"
#include "../forward_smt_veritas.h"
#include "../using_veritas.h"

namespace LINEAR_CONSTRAINTS_CHECKER { struct Specification; }

class ToVeritasVisitor final: public AstVisitorConst {

private:
    VERITAS_IN_PLAJA::Context& context;
    const StateIndexesInVeritas& stateIndexes;
    //
    std::unique_ptr<VeritasConstraint> rlt;

    // assignment mode:
    VeritasVarIndex_type assignmentTarget;
    VERITAS_IN_PLAJA::Equation::EquationType assignmentType;

    void visit(const ArrayAccessExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;

    // non-standard:
    void visit(const StateConditionExpression* exp) override;
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    explicit ToVeritasVisitor(const StateIndexesInVeritas& state_indexes_in_veritas);
public:
    ~ToVeritasVisitor() final;
    DELETE_CONSTRUCTOR(ToVeritasVisitor)

public:
    static std::unique_ptr<VeritasConstraint> to_veritas_constraint(const Expression& expr, const StateIndexesInVeritas& state_indexes);

    static std::unique_ptr<VeritasConstraint> to_assignment_constraint(VeritasVarIndex_type target_var, VERITAS_IN_PLAJA::Equation::EquationType assignment_type, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes);

};

namespace VERITAS_IN_PLAJA {
    extern std::unique_ptr<VeritasConstraint> to_veritas_constraint(const Expression& exp, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInVeritas& state_indexes);

    extern std::unique_ptr<VeritasConstraint> to_assignment_constraint(VeritasVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<VeritasConstraint> to_lb_constraint(VeritasVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInVeritas& state_indexes);
    extern std::unique_ptr<VeritasConstraint> to_ub_constraint(VeritasVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInVeritas& state_indexes);

    extern std::unique_ptr<VeritasConstraint> to_assignment_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasFloating_type scalar);
    extern std::unique_ptr<VeritasConstraint> to_copy_constraint(VERITAS_IN_PLAJA::Context& c, VeritasVarIndex_type target_var, VeritasVarIndex_type src_var);
}

#endif //PLAJA_TO_VERITAS_VISITOR_H
