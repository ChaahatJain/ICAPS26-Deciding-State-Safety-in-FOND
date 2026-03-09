//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "properties.h"
#include "../../visitor/ast_visitor.h"
#include "../../visitor/ast_visitor_const.h"
#include "../property.h"

Properties::Properties() = default;

Properties::~Properties() = default;

// construction:

void Properties::reserve(std::size_t props_cap) { properties.reserve(props_cap); }

void Properties::add_property(std::unique_ptr<Property>&& property) { properties.push_back(std::move(property)); }

void Properties::add_property(const Property& property) { add_property(property.deepCopy()); }

// setter:

void Properties::set_property(std::unique_ptr<Property>&& property, std::size_t index) {
    PLAJA_ASSERT(index < properties.size())
    properties[index] = std::move(property);
}

void Properties::set_property(Property& property, std::size_t index) { set_property(property.deepCopy(), index); }

//

std::unique_ptr<Properties> Properties::deepCopy() const {
    std::unique_ptr<Properties> copy(new Properties());
    // properties
    copy->reserve(get_number_properties());
    for (auto property_it = propertyIterator(); !property_it.end(); ++property_it) { copy->add_property(property_it->deepCopy()); }
    //
    return copy;
}

void Properties::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }
void Properties::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }
