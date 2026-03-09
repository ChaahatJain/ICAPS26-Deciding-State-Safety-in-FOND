//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "property.h"
#include "../visitor/ast_visitor.h"
#include "../visitor/ast_visitor_const.h"
#include "expression/property_expression.h"

Property::Property():
    name(PLAJA_UTILS::emptyString)
    , propertyExpression() {
}

Property::~Property() = default;

// setter:

void Property::set_propertyExpression(std::unique_ptr<PropertyExpression>&& prop_exp) { propertyExpression = std::move(prop_exp); }

//

std::unique_ptr<Property> Property::deepCopy() const {
    std::unique_ptr<Property> copy(new Property());
    copy->copy_comment(*this);
    copy->name = name;
    if (propertyExpression) { copy->propertyExpression = propertyExpression->deepCopy_PropExp(); }
    return copy;
}

// override:

void Property::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void Property::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

