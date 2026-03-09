//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "property_interval.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/expression.h"

PropertyInterval::PropertyInterval():
    lower(), lowerExclusive(false),
    upper(), upperExclusive(false) {
}

PropertyInterval::~PropertyInterval() = default;

// setter:

void PropertyInterval::set_lower(std::unique_ptr<Expression>&& lower_) {
    lower = std::move(lower_);
}

void PropertyInterval::set_upper(std::unique_ptr<Expression>&& upper_) {
    upper = std::move(upper_);
}

//

std::unique_ptr<PropertyInterval> PropertyInterval::deepCopy() const {
    std::unique_ptr<PropertyInterval> copy(new PropertyInterval());
    if (lower) copy->lower = lower->deepCopy_Exp();
    copy->lowerExclusive = lowerExclusive;
    if (upper) copy->upper = upper->deepCopy_Exp();
    copy->upperExclusive = upperExclusive;
    return copy;
}

// override

void PropertyInterval::accept(AstVisitor* astVisitor) {
    astVisitor->visit(this);
}

void PropertyInterval::accept(AstVisitorConst* astVisitor) const {
    astVisitor->visit(this);
}
