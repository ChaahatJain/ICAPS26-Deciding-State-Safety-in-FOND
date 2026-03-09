//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTY_H
#define PLAJA_PROPERTY_H


#include <memory>
#include "ast_element.h"
#include "commentable.h"

// forward declaration:
class PropertyExpression;

class Property: public AstElement, public Commentable {
private:
    std::string name;
    std::unique_ptr<PropertyExpression> propertyExpression;
public:
    Property();
    ~Property() override;

    // setter:
    inline void set_name(std::string name_) {name = std::move(name_);}
    void set_propertyExpression(std::unique_ptr<PropertyExpression>&& prop_exp);

    // getter:
    inline const std::string& get_name() const {return name;}
    inline PropertyExpression* get_propertyExpression() {return propertyExpression.get();}
    inline const PropertyExpression* get_propertyExpression() const {return propertyExpression.get();}

    /**
     * Deep copy of a property.
     * @return
     */
    std::unique_ptr<Property> deepCopy() const;

    // override:
    void accept(AstVisitor* astVisitor) override;
	void accept(AstVisitorConst* astVisitor) const override;
};


#endif //PLAJA_PROPERTY_H
