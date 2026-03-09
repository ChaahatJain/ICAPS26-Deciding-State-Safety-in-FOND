//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_ELEMENT_VISITOR_H
#define PLAJA_AST_ELEMENT_VISITOR_H

#include "ast_visitor.h"

class AstElementVisitor: public AstVisitor {

protected:
    AstElementVisitor();

public:
    ~AstElementVisitor() override = 0;
    DELETE_CONSTRUCTOR(AstElementVisitor)

    void visit(Action* action) override;
    void visit(Assignment* assignment) override;
    void visit(Automaton* automaton) override;
    void visit(Composition* sys) override;
    void visit(CompositionElement* sys_el) override;
    void visit(ConstantDeclaration* const_decl) override;
    void visit(Destination* destination) override;
    void visit(Edge* edge) override;
    void visit(Location* loc) override;
    void visit(Metadata* metadata) override;
    void visit(Model* model) override;
    void visit(Property* property) override;
    void visit(PropertyInterval* property_interval) override;
    void visit(RewardBound* reward_bound) override;
    void visit(RewardInstant* reward_instant) override;
    void visit(Synchronisation* sync) override;
    void visit(TransientValue* trans_val) override;
    void visit(VariableDeclaration* var_decl) override;
    /* Non-standard. */
    void visit(FreeVariableDeclaration* var_decl) override;
    void visit(Properties* properties) override;
    void visit(RewardAccumulation* reward_acc) override;

    /* Expressions. */
    void visit(ArrayAccessExpression* exp) override;
    void visit(ArrayConstructorExpression* exp) override;
    void visit(ArrayValueExpression* exp) override;
    void visit(BinaryOpExpression* exp) override;
    void visit(BoolValueExpression* exp) override;
    void visit(ConstantExpression* exp) override;
    void visit(DerivativeExpression* exp) override;
    void visit(DistributionSamplingExpression* exp) override;
    void visit(ExpectationExpression* exp) override;
    void visit(FilterExpression* exp) override;
    void visit(FreeVariableExpression* exp) override;
    void visit(IntegerValueExpression* exp) override;
    void visit(ITE_Expression* exp) override;
    void visit(PathExpression* exp) override;
    void visit(QfiedExpression* exp) override;
    void visit(RealValueExpression* exp) override;
    void visit(StatePredicateExpression* exp) override;
    void visit(UnaryOpExpression* exp) override;
    void visit(VariableExpression* exp) override;
    /* Non-standard. */
    void visit(ActionOpTuple* op) override;
    void visit(CommentExpression* op) override;
    void visit(LetExpression* exp) override;
    void visit(LocationValueExpression* exp) override;
    void visit(ObjectiveExpression* exp) override;
    void visit(PAExpression* exp) override;
    void visit(PredicatesExpression* exp) override;
    void visit(ProblemInstanceExpression* exp) override;
    void visit(StateConditionExpression* exp) override;
    void visit(StateValuesExpression* exp) override;
    void visit(StatesValuesExpression* exp) override;
    void visit(VariableValueExpression* exp) override;
    /* Special cases. */
    void visit(LinearExpression* exp) override;
    void visit(NaryExpression* exp) override;

    /* Types. */
    void visit(ArrayType* type) override;
    void visit(BoolType* type) override;
    void visit(BoundedType* type) override;
    void visit(ClockType* type) override;
    void visit(ContinuousType* type) override;
    void visit(IntType* type) override;
    void visit(RealType* type) override;
    /* Non-standard. */
    void visit(LocationType* type) override;

};

#endif //PLAJA_AST_ELEMENT_VISITOR_H
