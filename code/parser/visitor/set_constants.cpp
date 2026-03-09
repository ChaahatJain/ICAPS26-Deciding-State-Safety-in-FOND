//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2019) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef NDEBUG

#include "set_constants.h"
#include "../../exception/semantics_exception.h"
#include "../ast/expression/constant_expression.h"
#include "../ast/constant_declaration.h"
#include "ast_visitor.h"

class SetConstants final: public AstVisitor {

private:
    const std::unordered_map<ConstantIdType, const ConstantDeclaration*>* idToConst;

    void visit(ConstantExpression* exp) override;

public:
    explicit SetConstants(const std::unordered_map<ConstantIdType, const ConstantDeclaration*>& id_to_const);
    ~SetConstants() override;
    DELETE_CONSTRUCTOR(SetConstants)

};

/**********************************************************************************************************************/

SetConstants::SetConstants(const std::unordered_map<ConstantIdType, const ConstantDeclaration*>& id_to_const):
    idToConst(&id_to_const) {
}

SetConstants::~SetConstants() = default;

/* */

void SetConstants::visit(ConstantExpression* exp) {
    PLAJA_ASSERT(idToConst->count(exp->get_id()))
    const auto* value = idToConst->at(exp->get_id())->get_value();
    PLAJA_ASSERT(value->is_constant())
    PLAJA_ASSERT(exp->get_value() == value);
}

/**********************************************************************************************************************/

void SET_CONSTANTS::set_constants(const std::unordered_map<ConstantIdType, const ConstantDeclaration*>& id_to_const, AstElement& ast_el) {
    SetConstants set_constants(id_to_const);
    ast_el.accept(&set_constants);
}

#endif

