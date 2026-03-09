//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#ifndef PLAJA_SMT_BASED_RESTRICTIONS_CHECKER_H
#define PLAJA_SMT_BASED_RESTRICTIONS_CHECKER_H

#include <memory>
#include "../../../parser/visitor/ast_visitor_const.h"
#include "../../factories/forward_factories.h"
#include "../forward_smt_z3.h"

class SmtBasedRestrictionsChecker final: public AstVisitorConst {

private:
    const ModelZ3* modelZ3;
    std::unique_ptr<Z3_IN_PLAJA::SMTSolver> solver;

public:
    explicit SmtBasedRestrictionsChecker(const PLAJA::Configuration& config);
    ~SmtBasedRestrictionsChecker() override;
    DELETE_CONSTRUCTOR(SmtBasedRestrictionsChecker)

    void visit(const Assignment* assignment) override;

};

namespace SMT_BASED_RESTRICTIONS_CHECKER {

    extern void check(const AstElement& ast_element, const PLAJA::Configuration& config);

}

#endif //PLAJA_SMT_BASED_RESTRICTIONS_CHECKER_H
