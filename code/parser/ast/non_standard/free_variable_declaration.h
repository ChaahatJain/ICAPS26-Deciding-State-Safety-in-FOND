//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2021) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_FREE_VARIABLE_DECLARATION_H
#define PLAJA_FREE_VARIABLE_DECLARATION_H

#include <memory>
#include <utility>
#include "../../using_parser.h"
#include "../ast_element.h"
#include "../commentable.h"

// forward declaration:
class DeclarationType;

/**
 * In JANI standard this is simply an identifier.
 */
class FreeVariableDeclaration: public AstElement, public Commentable {

private:
    std::string name;
    std::unique_ptr<DeclarationType> declarationType;

    FreeVariableID_type varID;

public:
    explicit FreeVariableDeclaration(FreeVariableID_type var_id);
    ~FreeVariableDeclaration() override;

    // setter:
    inline void set_name(const std::string& name_) { name = name_; }
    void set_type(std::unique_ptr<DeclarationType>&& decl_type);

    // getter:
    [[nodiscard]] inline const std::string& get_name() const { return name; }
    inline DeclarationType* get_type() { return declarationType.get(); }
    [[nodiscard]] inline const DeclarationType* get_type() const { return declarationType.get(); }
    [[nodiscard]] inline FreeVariableID_type get_id() const { return varID; }

    /**
     * Deep copy of a variable declaration.
     * @return
     */
    [[nodiscard]] std::unique_ptr<FreeVariableDeclaration> deepCopy() const;

    // override:
    void accept(AstVisitor* ast_visitor) override;
    void accept(AstVisitorConst* ast_visitor) const override;
};


#endif //PLAJA_FREE_VARIABLE_DECLARATION_H
