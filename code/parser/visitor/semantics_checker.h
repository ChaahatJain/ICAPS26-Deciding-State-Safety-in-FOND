//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SEMANTICS_CHECKER_H
#define PLAJA_SEMANTICS_CHECKER_H

#include "ast_visitor.h"


/***
 * Class to implement semantic checks.
 * However, for simplicity some checks are done in ModelInformation, i.p., w.r.t variables and constants.
 */
class SemanticsChecker: public AstVisitor {
private:
    const Model* model;

    explicit SemanticsChecker(const Model* model);
public:
    ~SemanticsChecker() override;
    DELETE_CONSTRUCTOR(SemanticsChecker)

    static void check_semantics(AstElement* ast_element); // require call on complete model for transient-values and specialized check on array access
                                                          // yet, we allow call on any ast element for testing purposes

    void visit(Assignment* assignment) override;
    void visit(Automaton* automaton) override;
    void visit(ConstantDeclaration* const_decl) override;
    void visit(Destination* destination) override;
    void visit(Edge* edge) override;
    void visit(Location* loc) override;
    void visit(Model* model) override;
    void visit(PropertyInterval* property_interval) override;
    void visit(RewardBound* reward_bound) override;
    void visit(RewardInstant* reward_instant) override;
    void visit(TransientValue* trans_val) override;
    void visit(VariableDeclaration* var_decl) override;

    // expressions:
    void visit(ArrayAccessExpression* exp) override;
    void visit(ArrayConstructorExpression* exp) override;
    void visit(ArrayValueExpression* exp) override;
    void visit(BinaryOpExpression* exp) override;
    void visit(DerivativeExpression* exp) override;
    void visit(DistributionSamplingExpression* exp) override;
    void visit(ExpectationExpression* exp) override;
    void visit(FilterExpression* exp) override;
    void visit(ITE_Expression* exp) override;
    void visit(PathExpression* exp) override;
    void visit(QfiedExpression* exp) override;
    void visit(UnaryOpExpression* exp) override;
    // non-standard:
    void visit(ConstantArrayAccessExpression* exp) override;
    void visit(ObjectiveExpression* exp) override;
    void visit(PAExpression* exp) override;
    void visit(PredicatesExpression* exp) override;
    void visit(StateConditionExpression* exp) override;
    void visit(VariableValueExpression* exp) override;
    void visit(StateValuesExpression* exp) override;
    // void visit(StatesValuesExpression* exp) override; might warn for duplicate state_values_expression

    // types:
    void visit(BoundedType* type) override;

};

// To enable parsing without including all the visitor stuff:
namespace SEMANTICS_CHECKER { extern void check_semantics(AstElement* ast_element); }

#endif //PLAJA_SEMANTICS_CHECKER_H
