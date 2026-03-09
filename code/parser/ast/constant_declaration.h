//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_CONSTANT_DECLARATION_H
#define PLAJA_CONSTANT_DECLARATION_H

#include <memory>
#include "../using_parser.h"
#include "ast_element.h"
#include "commentable.h"

class Expression;

class DeclarationType;

class ConstantDeclaration: public AstElement, public Commentable {
private:

    std::string name;
    ConstantIdType id; // Unique among all constants, larger equal zero and smaller than number of constants.
    std::unique_ptr<DeclarationType> declarationType; // Basic type or bounded type (or array with non-standard support).
    std::unique_ptr<Expression> value;

public:
    explicit ConstantDeclaration(ConstantIdType id);
    ~ConstantDeclaration() override;
    DELETE_CONSTRUCTOR(ConstantDeclaration)

    /* Setter. */

    inline void set_name(std::string name_c) { name = std::move(name_c); }

    // inline void set_id(int id_n) { id = id_n; }

    void set_declaration_type(std::unique_ptr<DeclarationType>&& decl_type);

    void set_value(std::unique_ptr<Expression>&& val);

    /* Getter. */

    [[nodiscard]] inline const std::string& get_name() const { return name; }

    [[nodiscard]] inline ConstantIdType get_id() const { return id; }

    [[nodiscard]] inline DeclarationType* get_type() { return declarationType.get(); }

    [[nodiscard]] inline const DeclarationType* get_type() const { return declarationType.get(); }

    [[nodiscard]] inline Expression* get_value() { return value.get(); }

    [[nodiscard]] inline const Expression* get_value() const { return value.get(); }

    /** Deep copy of a constant declaration. */
    [[nodiscard]] std::unique_ptr<ConstantDeclaration> deep_copy() const;

    /* Override. */
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
};

#endif //PLAJA_CONSTANT_DECLARATION_H
