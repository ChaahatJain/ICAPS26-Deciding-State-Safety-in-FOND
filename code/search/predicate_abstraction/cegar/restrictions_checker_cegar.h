//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_RESTRICTIONS_CHECKER_CEGAR_H
#define PLAJA_RESTRICTIONS_CHECKER_CEGAR_H

#include <unordered_set>
#include "../../../parser/using_parser.h"
#include "../../smt/visitor/restrictions_smt_checker.h"

/**
 * Enforce additional restrictions for CEGAR.
 */
class RestrictionsCheckerCegar final: public RestrictionsSMTChecker {

    std::unordered_set<VariableID_type> nonDetUpdatedVars;

    using RestrictionsSMTChecker::visit;
    void visit(const Assignment* assignment) override;
    void visit(const Edge* edge) override;

    RestrictionsCheckerCegar();
public:
    ~RestrictionsCheckerCegar() override;
    DELETE_CONSTRUCTOR(RestrictionsCheckerCegar)

    /**
     * @param catch_exception : whether not-supported-exceptions shall be caught.
     */
    static bool check_restrictions(const AstElement* ast_element, bool catch_exception = true);
    static bool check_restrictions(const Model* model, bool catch_exception = true);

};

#endif //PLAJA_RESTRICTIONS_CHECKER_CEGAR_H
