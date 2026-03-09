//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_PROPERTY_EXPRESSION_H
#define PLAJA_PROPERTY_EXPRESSION_H

#include <memory>
#include "../ast_element.h"

// forward declaration:
class DeclarationType;

class PropertyExpression: public AstElement {
protected:
    std::unique_ptr<DeclarationType> resultType;
public:
    PropertyExpression();
    ~PropertyExpression() override = 0;

    /**
     * Is this expression a proposition,
     * i.e., something that can be evaluated over a state?
     * In JANI this holds exactly for the expression subclass.
    */
    [[nodiscard]] virtual bool is_proposition() const;

    /**
    * Get the type the expression evaluates to. Type is first computed if necessary and stored as result type.
    * Note: the type of subclasses is only determined if necessary. In particular, the expressions are assumed to be well-typed.
    * *nullptr* indicates an unspecified type, e.g., 'set of bool' in a special case of the filter property expression.
    * Throws *SemanticsException* if for a subclass of Expression no type can be determined.
    */
    virtual const DeclarationType* determine_type();

    /**
    * Get the type the expression evaluates to, if already computed, or nullptr.
    */
    [[nodiscard]] const DeclarationType* get_type() const;

    /**
     * Deep copy of property expressions.
     */
    [[nodiscard]] virtual std::unique_ptr<PropertyExpression> deepCopy_PropExp() const = 0;
    [[nodiscard]] virtual std::unique_ptr<PropertyExpression> move_prop_exp();

    [[nodiscard]] std::unique_ptr<AstElement> deep_copy_ast() const final;
    [[nodiscard]] std::unique_ptr<AstElement> move_ast() final;

    static std::unique_ptr<class Property> move_to_property(std::unique_ptr<PropertyExpression>&& expr, const std::string& name);
    static std::unique_ptr<class Properties> move_to_properties(std::unique_ptr<PropertyExpression>&& expr, const std::string& name);
    static void write_to_properties(std::unique_ptr<PropertyExpression>&& expr, const std::string& filename);

};

#endif //PLAJA_PROPERTY_EXPRESSION_H
