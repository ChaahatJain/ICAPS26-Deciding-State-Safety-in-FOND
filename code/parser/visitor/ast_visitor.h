//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_VISITOR_H
#define PLAJA_AST_VISITOR_H

#include <memory>
#include "../../utils/default_constructors.h"
#include "../../utils/utils.h"
#include "../using_parser.h"
#include "../ast_forward_declarations.h"

class AstVisitor {
protected:
    std::unique_ptr<AstElement> result; // result ast element
    bool replace_by_result; // if true replace last visited element by result
    FIELD_IF_DEBUG(AutomatonIndex_type automatonIndex;) // Automaton index w.r.t. location variable index (may be useful in derived classes).

    // replacement aux:

    inline void set_to_replace(std::unique_ptr<AstElement>&& ast_element) {
        replace_by_result = true;
        result = std::move(ast_element);
    }

    template<typename AstElement_type>
    inline std::unique_ptr<AstElement_type> move_result() {
        PLAJA_ASSERT(replace_by_result and result.get())
        replace_by_result = false;
        return PLAJA_UTILS::cast_unique<AstElement_type>(std::move(result));
    }

#define AST_ACCEPT_IF(GET, SET, AST_ELEMENT_TYPE) \
    {                                              \
        auto* element = GET();\
        if (element) {\
            element->accept(this);\
            if (replace_by_result) { SET(move_result<AST_ELEMENT_TYPE>()); }\
        }\
    }

#define AST_ITERATOR(IT, AST_ELEMENT_TYPE) \
         for (auto it = IT; !it.end(); ++it) {\
            it->accept(this);\
            if (replace_by_result) { it.set(move_result<AST_ELEMENT_TYPE>()); } \
         }
protected:

    static AstElement* cast_model(Model* model);

    AstVisitor();
public:
    virtual ~AstVisitor() = 0;
    DELETE_CONSTRUCTOR(AstVisitor)

    virtual void visit(Action* action_label);
    virtual void visit(Assignment* assignment);
    virtual void visit(Automaton* automaton);
    virtual void visit(Composition* sys);
    virtual void visit(CompositionElement* sys_el);
    virtual void visit(ConstantDeclaration* const_decl);
    virtual void visit(Destination* destination);
    virtual void visit(Edge* edge);
    virtual void visit(Location* loc);
    virtual void visit(Metadata* metadata);
    virtual void visit(Model* model);
    virtual void visit(Property* property);
    virtual void visit(PropertyInterval* property_interval);
    virtual void visit(RewardBound* reward_bound);
    virtual void visit(RewardInstant* reward_instant);
    virtual void visit(Synchronisation* sync);
    virtual void visit(TransientValue* trans_val);
    virtual void visit(VariableDeclaration* var_decl);
    /* Non-standard. */
    virtual void visit(FreeVariableDeclaration* var_decl);
    virtual void visit(Properties* properties);
    virtual void visit(RewardAccumulation* reward_acc);

    /* Expressions. */
    virtual void visit(ArrayAccessExpression* exp);
    virtual void visit(ArrayConstructorExpression* exp);
    virtual void visit(ArrayValueExpression* exp);
    virtual void visit(BinaryOpExpression* exp);
    virtual void visit(BoolValueExpression* exp);
    virtual void visit(ConstantExpression* exp);
    virtual void visit(DerivativeExpression* exp);
    virtual void visit(DistributionSamplingExpression* exp);
    virtual void visit(ExpectationExpression* exp);
    virtual void visit(FilterExpression* exp);
    virtual void visit(FreeVariableExpression* exp);
    virtual void visit(IntegerValueExpression* exp);
    virtual void visit(ITE_Expression* exp);
    virtual void visit(PathExpression* exp);
    virtual void visit(QfiedExpression* exp);
    virtual void visit(RealValueExpression* exp);
    virtual void visit(StatePredicateExpression* exp);
    virtual void visit(UnaryOpExpression* exp);
    virtual void visit(VariableExpression* exp);
    /* Non-standard. */
    virtual void visit(ActionOpTuple* op);
    virtual void visit(ConstantArrayAccessExpression* exp);
    virtual void visit(CommentExpression* op);
    virtual void visit(LetExpression* exp);
    virtual void visit(LocationValueExpression* exp);
    virtual void visit(ObjectiveExpression* exp);
    virtual void visit(PAExpression* exp);
    virtual void visit(PredicatesExpression* exp);
    virtual void visit(ProblemInstanceExpression* exp);
    virtual void visit(StateConditionExpression* exp);
    virtual void visit(StateValuesExpression* exp);
    virtual void visit(StatesValuesExpression* exp);
    virtual void visit(VariableValueExpression* exp);
    /* Special case. */
    virtual void visit(LinearExpression* exp);
    virtual void visit(NaryExpression* exp);

    /* Types. */
    virtual void visit(ArrayType* type);
    virtual void visit(BoolType* type);
    virtual void visit(BoundedType* type);
    virtual void visit(ClockType* type);
    virtual void visit(ContinuousType* type);
    virtual void visit(IntType* type);
    virtual void visit(RealType* type);
    /* Non-standard. */
    virtual void visit(LocationType* type);

};

#endif //PLAJA_AST_VISITOR_H
