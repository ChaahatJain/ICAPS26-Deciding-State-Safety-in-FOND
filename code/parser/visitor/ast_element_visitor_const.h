//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_ELEMENT_VISITOR_CONST_H
#define PLAJA_AST_ELEMENT_VISITOR_CONST_H

#include "ast_visitor_const.h"

class AstElementVisitorConst: public AstVisitorConst {

protected:
    AstElementVisitorConst();

public:
    ~AstElementVisitorConst() override = 0;
    DELETE_CONSTRUCTOR(AstElementVisitorConst)

    void visit(const Action* action) override;
    void visit(const Assignment* assignment) override;
    void visit(const Automaton* automaton) override;
    void visit(const Composition* sys) override;
    void visit(const CompositionElement* sys_el) override;
    void visit(const ConstantDeclaration* const_decl) override;
    void visit(const Destination* destination) override;
    void visit(const Edge* edge) override;
    void visit(const Location* loc) override;
    void visit(const Metadata* metadata) override;
    void visit(const Model* model) override;
    void visit(const Property* property) override;
    void visit(const PropertyInterval* property_interval) override;
    void visit(const RewardBound* reward_bound) override;
    void visit(const RewardInstant* reward_instant) override;
    void visit(const Synchronisation* sync) override;
    void visit(const TransientValue* trans_val) override;
    void visit(const VariableDeclaration* var_decl) override;
    /* Non-standard. */
    void visit(const FreeVariableDeclaration* var_decl) override;
    void visit(const Properties* properties) override;
    void visit(const RewardAccumulation* reward_acc) override;

    /* Expressions. */
    void visit(const ArrayAccessExpression* exp) override;
    void visit(const ArrayConstructorExpression* exp) override;
    void visit(const ArrayValueExpression* exp) override;
    void visit(const BinaryOpExpression* exp) override;
    void visit(const BoolValueExpression* exp) override;
    void visit(const ConstantExpression* exp) override;
    void visit(const DerivativeExpression* exp) override;
    void visit(const DistributionSamplingExpression* exp) override;
    void visit(const ExpectationExpression* exp) override;
    void visit(const FilterExpression* exp) override;
    void visit(const FreeVariableExpression* exp) override;
    void visit(const IntegerValueExpression* exp) override;
    void visit(const ITE_Expression* exp) override;
    void visit(const PathExpression* exp) override;
    void visit(const QfiedExpression* exp) override;
    void visit(const RealValueExpression* exp) override;
    void visit(const StatePredicateExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;
    void visit(const VariableExpression* exp) override;
    /* Non-standard. */
    void visit(const ActionOpTuple* op) override;
    void visit(const CommentExpression* op) override;
    void visit(const LetExpression* exp) override;
    void visit(const LocationValueExpression* exp) override;
    void visit(const ObjectiveExpression* exp) override;
    void visit(const PAExpression* exp) override;
    void visit(const PredicatesExpression* exp) override;
    void visit(const ProblemInstanceExpression* exp) override;
    void visit(const StateConditionExpression* exp) override;
    void visit(const StateValuesExpression* exp) override;
    void visit(const StatesValuesExpression* exp) override;
    void visit(const VariableValueExpression* exp) override;
    /* Special cases. */
    void visit(const LinearExpression* exp) override;
    void visit(const NaryExpression* exp) override;

    /* Types. */
    void visit(const ArrayType* type) override;
    void visit(const BoolType* type) override;
    void visit(const BoundedType* type) override;
    void visit(const ClockType* type) override;
    void visit(const ContinuousType* type) override;
    void visit(const IntType* type) override;
    void visit(const RealType* type) override;
    /* Non-standard. */
    void visit(const LocationType* type) override;

};

#endif //PLAJA_AST_ELEMENT_VISITOR_CONST_H
