//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RESTRICTIONS_CHECKER_EXPLICIT_H
#define PLAJA_RESTRICTIONS_CHECKER_EXPLICIT_H

#include "../../parser/visitor/ast_visitor_const.h"

/**
 * Enforce additional restriction for predicate abstraction.
 */
class RestrictionsCheckerExplicit: public AstVisitorConst {

    RestrictionsCheckerExplicit();
public:
    ~RestrictionsCheckerExplicit() override;
    DELETE_CONSTRUCTOR(RestrictionsCheckerExplicit)

    /**
     * @param catch_exception : whether not-supported-exceptions shall be caught.
     */
    static bool check_restrictions(const AstElement* ast_element, bool catch_exception = true);
    static bool check_restrictions(const Model* model, bool catch_exception = true);

    void visit(const Assignment* assignment) override;
    void visit(const VariableDeclaration* var_decl) override;

};

#endif //PLAJA_RESTRICTIONS_CHECKER_EXPLICIT_H
