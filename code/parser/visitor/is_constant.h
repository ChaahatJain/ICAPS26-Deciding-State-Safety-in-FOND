//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_IS_CONSTANT_H
#define PLAJA_IS_CONSTANT_H

#include <unordered_map>
#include <unordered_set>
#include "ast_visitor_const.h"
#include "../using_parser.h"

class IsConstant final: public AstVisitorConst {
private:
    std::unordered_set<VariableID_type> nonConstantVars;

    void visit(const Assignment* assignment) override;
    void visit(const VariableDeclaration* var_decl) override;
    void visit(const DerivativeExpression* exp) override;

public:
    explicit IsConstant(const Model& model);
    ~IsConstant() override;
    DELETE_CONSTRUCTOR(IsConstant)

    [[nodiscard]] inline const std::unordered_set<VariableID_type>& get_non_constant_vars() const { return nonConstantVars; }

    [[nodiscard]] bool is_constant(VariableID_type var_id) const;
    static bool is_constant(VariableID_type var_id, const Model& model);
};

namespace SET_CONSTS {

    extern void transform_to_consts(const std::unordered_map<VariableID_type, const ConstantDeclaration*>& id_to_consts, AstElement& ast_element);

    extern void transform_const_vars(Model& model);

}

#endif //PLAJA_IS_CONSTANT_H
