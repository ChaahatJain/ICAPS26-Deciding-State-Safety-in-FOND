//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_TO_MARABOU_VISITOR_H
#define PLAJA_TO_MARABOU_VISITOR_H

#include <memory>
#include "../../../include/marabou_include/equation.h"
#include "../../../parser/visitor/ast_visitor_const.h"
#include "../forward_smt_nn.h"
#include "../using_marabou.h"

namespace LINEAR_CONSTRAINTS_CHECKER { struct Specification; }

class ToMarabouVisitor final: public AstVisitorConst {

private:
    MARABOU_IN_PLAJA::Context& context;
    const StateIndexesInMarabou& stateIndexes;
    //
    std::unique_ptr<MarabouConstraint> rlt;

    // assignment mode:
    MarabouVarIndex_type assignmentTarget;
    Equation::EquationType assignmentType;

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

    explicit ToMarabouVisitor(const StateIndexesInMarabou& state_indexes_in_marabou);
public:
    ~ToMarabouVisitor() final;
    DELETE_CONSTRUCTOR(ToMarabouVisitor)

public:
    static std::unique_ptr<MarabouConstraint> to_marabou_constraint(const Expression& expr, const StateIndexesInMarabou& state_indexes);

    static std::unique_ptr<MarabouConstraint> to_assignment_constraint(MarabouVarIndex_type target_var, Equation::EquationType assignment_type, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes);

};

namespace MARABOU_IN_PLAJA {
    extern std::unique_ptr<MarabouConstraint> to_marabou_constraint(const Expression& exp, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<PredicateConstraint> to_predicate_constraint(const Expression& constraint, const StateIndexesInMarabou& state_indexes);

    extern std::unique_ptr<MarabouConstraint> to_assignment_constraint(MarabouVarIndex_type target_var, const Expression& assignment_sum, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<MarabouConstraint> to_lb_constraint(MarabouVarIndex_type target_var, const Expression& lb_sum, const StateIndexesInMarabou& state_indexes);
    extern std::unique_ptr<MarabouConstraint> to_ub_constraint(MarabouVarIndex_type target_var, const Expression& ub_sum, const StateIndexesInMarabou& state_indexes);

    extern std::unique_ptr<MarabouConstraint> to_assignment_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouFloating_type scalar);
    extern std::unique_ptr<MarabouConstraint> to_copy_constraint(MARABOU_IN_PLAJA::Context& c, MarabouVarIndex_type target_var, MarabouVarIndex_type src_var);
}

#endif //PLAJA_TO_MARABOU_VISITOR_H
