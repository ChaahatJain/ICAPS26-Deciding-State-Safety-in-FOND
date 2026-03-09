//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "transient_value.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/lvalue_expression.h"

TransientValue::TransientValue():
    ref()
    , value() {
}

TransientValue::~TransientValue() = default;

// setter:

void TransientValue::set_ref(std::unique_ptr<LValueExpression>&& ref_) { ref = std::move(ref_); }

void TransientValue::set_value(std::unique_ptr<Expression>&& val) { value = std::move(val); }

//

std::unique_ptr<TransientValue> TransientValue::deepCopy() const {
    std::unique_ptr<TransientValue> copy(new TransientValue());
    copy->copy_comment(*this);
    if (ref) { copy->ref = ref->deep_copy_lval_exp(); }
    if (value) { copy->value = value->deepCopy_Exp(); }
    return copy;
}

//

// source is typically just a const view of target, this is valid as transient variables are assigned by transient-variable-free expressions
void TransientValue::evaluate(const StateBase* source, StateBase* target) const {
    PLAJA_ASSERT(target)
    ref->assign(*target, source, value.get());
}

// override:

void TransientValue::accept(AstVisitor* astVisitor) { astVisitor->visit(this); }

void TransientValue::accept(AstVisitorConst* astVisitor) const { astVisitor->visit(this); }



