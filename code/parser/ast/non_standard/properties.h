//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2022) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTIES_H
#define PLAJA_PROPERTIES_H

#include <memory>
#include <vector>
#include "../iterators/ast_iterator.h"
#include "../ast_element.h"

// forward declaration:
class Property;

/** Auxiliary class to generate "additional properties" file */
class Properties final: public AstElement {

private:
    std::vector<std::unique_ptr<Property>> properties;

public:
    Properties();
    ~Properties() override;

    // construction:
    void reserve(std::size_t props_cap);
    void add_property(std::unique_ptr<Property>&& property);
    void add_property(const Property& property);

    // setter:
    void set_property(std::unique_ptr<Property>&& property, std::size_t index);
    void set_property(Property& property, std::size_t index);

    // getter:
    [[nodiscard]] inline std::size_t get_number_properties() const { return properties.size(); }
    [[nodiscard]] inline const Property* get_property(std::size_t index) const { return properties[index].get(); }
    [[nodiscard]] inline Property* get_property(std::size_t index) { return properties[index].get(); }
    [[nodiscard]] inline AstConstIterator<Property> propertyIterator() const { return AstConstIterator(properties); }
    [[nodiscard]] inline AstIterator<Property> propertyIterator() { return AstIterator(properties); }

    /**
    * Deep copy of an action.
    * @return
    */
    [[nodiscard]] std::unique_ptr<Properties> deepCopy() const;

    // override:
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;

};


#endif //PLAJA_PROPERTIES_H
