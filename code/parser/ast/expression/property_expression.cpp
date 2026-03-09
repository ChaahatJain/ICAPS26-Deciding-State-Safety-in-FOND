//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "property_expression.h"
#include "../../../exception/not_implemented_exception.h"
#include "../non_standard/properties.h"
#include "../type/declaration_type.h"
#include "../property.h"

PropertyExpression::PropertyExpression():
    resultType(nullptr) {}

PropertyExpression::~PropertyExpression() = default;

bool PropertyExpression::is_proposition() const { return false; } // Holds for all property expressions except expression subclass.

const DeclarationType* PropertyExpression::determine_type() { return resultType.get(); }

const DeclarationType* PropertyExpression::get_type() const { return resultType.get(); }

std::unique_ptr<PropertyExpression> PropertyExpression::move_prop_exp() { throw NotImplementedException(__PRETTY_FUNCTION__); }

std::unique_ptr<AstElement> PropertyExpression::deep_copy_ast() const { return deepCopy_PropExp(); }

std::unique_ptr<AstElement> PropertyExpression::move_ast() { return move_prop_exp(); }

std::unique_ptr<class Property> PropertyExpression::move_to_property(std::unique_ptr<PropertyExpression>&& expr, const std::string& name) {
    std::unique_ptr<Property> property(new Property());
    property->set_name(name);
    property->set_propertyExpression(std::move(expr));
    return property;
}

std::unique_ptr<class Properties> PropertyExpression::move_to_properties(std::unique_ptr<PropertyExpression>&& expr, const std::string& name) {
    std::unique_ptr<Properties> properties(new Properties());
    properties->reserve(1);
    properties->add_property(move_to_property(std::move(expr), name));
    return properties;
}

void PropertyExpression::write_to_properties(std::unique_ptr<PropertyExpression>&& expr, const std::string& filename) {
    const auto properties = move_to_properties(std::move(expr), filename);
    properties->write_to_file(filename);
}