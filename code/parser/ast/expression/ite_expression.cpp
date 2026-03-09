//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "ite_expression.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../type/declaration_type.h"

ITE_Expression::ITE_Expression() = default;

ITE_Expression::~ITE_Expression() = default;

// override:

PLAJA::integer ITE_Expression::evaluateInteger(const StateBase* state) const {
    JANI_ASSERT(condition and consequence and alternative)
    return condition->evaluateInteger(state) ? consequence->evaluateInteger(state) : alternative->evaluateInteger(state);
}

PLAJA::floating ITE_Expression::evaluateFloating(const StateBase* state) const {
    JANI_ASSERT(condition and consequence and alternative)
    PLAJA_ASSERT(resultType)
    if (resultType->is_floating_type()) {
        return condition->evaluateInteger(state) ? consequence->evaluateFloating(state) : alternative->evaluateFloating(state);
    } else {
        return evaluateInteger(state);
    }
}

bool ITE_Expression::is_constant() const {
    JANI_ASSERT(condition and consequence and alternative)
    return condition->is_constant() and consequence->is_constant() and alternative->is_constant();
}

bool ITE_Expression::equals(const Expression* exp) const {
    JANI_ASSERT(condition and consequence and alternative)
    auto* other = PLAJA_UTILS::cast_ptr_if<ITE_Expression>(exp);
    if (not other) { return false; }

    JANI_ASSERT(other->condition and other->consequence and other->alternative)
    return this->condition->equals(other->condition.get()) and
           this->consequence->equals(other->consequence.get()) and
           this->alternative->equals(other->alternative.get());
}

std::size_t ITE_Expression::hash() const {
    JANI_ASSERT(condition and consequence and alternative)
    constexpr unsigned prime = 31;
    std::size_t result = 1;
    result = prime * result + (condition->hash());
    result = prime * result + (consequence->hash());
    result = prime * result + (alternative->hash());
    return result;
}

const DeclarationType* ITE_Expression::determine_type() {
    if (resultType) { return resultType.get(); }
    else {
        JANI_ASSERT(consequence and alternative)
        if (consequence->determine_type()->is_assignable_from(*alternative->determine_type())) {
            return (resultType = consequence->get_type()->deep_copy_decl_type()).get();
        } else {
            JANI_ASSERT(alternative->get_type()->is_assignable_from(*consequence->get_type())) // must hold by invariant for well constructed ite expression
            return (resultType = alternative->get_type()->deep_copy_decl_type()).get();
        }
    }
}

void ITE_Expression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ITE_Expression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<Expression> ITE_Expression::deepCopy_Exp() const {
    std::unique_ptr<ITE_Expression> copy(new ITE_Expression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    if (condition) { copy->set_condition(condition->deepCopy_Exp()); }
    if (consequence) { copy->set_consequence(consequence->deepCopy_Exp()); }
    if (alternative) { copy->set_alternative(alternative->deepCopy_Exp()); }
    return copy;
}










