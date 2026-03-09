//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_VISITOR_CONST_H
#define PLAJA_AST_VISITOR_CONST_H

#include "../../utils/default_constructors.h"
#include "../ast_forward_declarations.h"

class AstVisitorConst {

#define AST_CONST_ACCEPT_IF(GET, AST_ELEMENT_TYPE) \
    {                                              \
        auto* element = GET;\
        if (element) { element->accept(this); }\
    }

#define AST_CONST_ITERATOR(IT, AST_ELEMENT_TYPE) \
         for (auto it = IT; !it.end(); ++it) { it->accept(this); }

protected:

    static const AstElement* cast_model(const Model* model);

    AstVisitorConst();
public:
    virtual ~AstVisitorConst() = 0;
    DEFAULT_CONSTRUCTOR(AstVisitorConst)

    virtual void visit(const Action* action);
    virtual void visit(const Assignment* assignment);
    virtual void visit(const Automaton* automaton);
    virtual void visit(const Composition* sys);
    virtual void visit(const CompositionElement* sys_el);
    virtual void visit(const ConstantDeclaration* const_decl);
    virtual void visit(const Destination* destination);
    virtual void visit(const Edge* edge);
    virtual void visit(const Location* loc);
    virtual void visit(const Metadata* metadata);
    virtual void visit(const Model* model);
    virtual void visit(const Property* property);
    virtual void visit(const PropertyInterval* property_interval);
    virtual void visit(const RewardBound* reward_bound);
    virtual void visit(const RewardInstant* reward_instant);
    virtual void visit(const Synchronisation* sync);
    virtual void visit(const TransientValue* trans_val);
    virtual void visit(const VariableDeclaration* var_decl);
    /* Non-standard. */
    virtual void visit(const FreeVariableDeclaration* var_decl);
    virtual void visit(const Properties* properties);
    virtual void visit(const RewardAccumulation* reward_acc);

    /* Expressions. */
    virtual void visit(const ArrayAccessExpression* exp);
    virtual void visit(const ArrayConstructorExpression* exp);
    virtual void visit(const ArrayValueExpression* exp);
    virtual void visit(const BinaryOpExpression* exp);
    virtual void visit(const BoolValueExpression* exp);
    virtual void visit(const ConstantExpression* exp);
    virtual void visit(const DerivativeExpression* exp);
    virtual void visit(const DistributionSamplingExpression* exp);
    virtual void visit(const ExpectationExpression* exp);
    virtual void visit(const FilterExpression* exp);
    virtual void visit(const FreeVariableExpression* exp);
    virtual void visit(const IntegerValueExpression* exp);
    virtual void visit(const ITE_Expression* exp);
    virtual void visit(const PathExpression* exp);
    virtual void visit(const QfiedExpression* exp);
    virtual void visit(const RealValueExpression* exp);
    virtual void visit(const StatePredicateExpression* exp);
    virtual void visit(const UnaryOpExpression* exp);
    virtual void visit(const VariableExpression* exp);
    /* Non-standard. */
    virtual void visit(const ActionOpTuple* op);
    virtual void visit(const ConstantArrayAccessExpression* exp);
    virtual void visit(const CommentExpression* op);
    virtual void visit(const LetExpression* exp);
    virtual void visit(const LocationValueExpression* exp);
    virtual void visit(const ObjectiveExpression* exp);
    virtual void visit(const PAExpression* exp);
    virtual void visit(const PredicatesExpression* exp);
    virtual void visit(const ProblemInstanceExpression* exp);
    virtual void visit(const StateConditionExpression* exp);
    virtual void visit(const StateValuesExpression* exp);
    virtual void visit(const StatesValuesExpression* exp);
    virtual void visit(const VariableValueExpression* exp);
    /* Special cases. */
    virtual void visit(const LinearExpression* exp);
    virtual void visit(const NaryExpression* exp);

    /* Types. */
    virtual void visit(const ArrayType* type);
    virtual void visit(const BoolType* type);
    virtual void visit(const BoundedType* type);
    virtual void visit(const ClockType* type);
    virtual void visit(const ContinuousType* type);
    virtual void visit(const IntType* type);
    virtual void visit(const RealType* type);
    /* Non-standard. */
    virtual void visit(const LocationType* type);

};

#endif //PLAJA_AST_VISITOR_CONST_H
