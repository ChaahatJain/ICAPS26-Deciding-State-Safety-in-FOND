//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_AST_ELEMENT_H
#define PLAJA_AST_ELEMENT_H

#include <memory>
#include <string>
#include "../../assertions.h"

class AstVisitor;

class AstVisitorConst;

class AstElement {
public:
    AstElement();
    virtual ~AstElement() = 0;
    DELETE_CONSTRUCTOR(AstElement)

    virtual void accept(AstVisitor* ast_visitor) = 0;
    virtual void accept(AstVisitorConst* ast_visitor) const = 0;
    [[nodiscard]] virtual std::unique_ptr<AstElement> deep_copy_ast() const;
    [[nodiscard]] virtual std::unique_ptr<AstElement> move_ast();

    [[nodiscard]] std::string to_string(bool readable = false) const;
    void dump(bool readable = false) const;
    void write_to_file(const std::string& filename) const;
};

#endif //PLAJA_AST_ELEMENT_H
