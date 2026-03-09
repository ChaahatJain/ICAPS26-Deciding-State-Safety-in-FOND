//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RESTRICTIONS_PA_CHECKER_H
#define PLAJA_RESTRICTIONS_PA_CHECKER_H

#include "../../../parser/visitor/ast_visitor_const.h"

/**
 * Enforce additional restriction for SMT (with Z3).
 */
class RestrictionsSMTChecker: public AstVisitorConst {

protected:
    RestrictionsSMTChecker();

public:
    ~RestrictionsSMTChecker() override;
    DELETE_CONSTRUCTOR(RestrictionsSMTChecker)

    /**
     *
     * @param ast_element
     * @param catch_exception, whether not-supported-exceptions shall be caught.
     * @return true if no restrictions are violated, false otherwise.
     */
    static bool check_restrictions(const AstElement* ast_element, bool catch_exception = true);

    void visit(const Assignment* assignment) override;
    void visit(const VariableDeclaration* var_decl) override; // If we forbid transient variables, there can be no transient values, i.e., we must not check locations.

    void visit(const BinaryOpExpression* exp) override;
    void visit(const UnaryOpExpression* exp) override;

};

#endif //PLAJA_RESTRICTIONS_PA_CHECKER_H
