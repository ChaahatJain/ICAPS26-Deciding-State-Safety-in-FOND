//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2024) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "smt_vars_of.h"
#include "../../../include/ct_config_const.h"
#include "../../../parser/ast/expression/non_standard/constant_array_access_expression.h"
#include "../../../parser/ast/expression/variable_expression.h"
#include "../../../parser/ast/destination.h"
#include "../../../parser/ast/edge.h"
#include "../../../parser/visitor/ast_visitor_const.h"
#include "../model/model_z3.h"
#include "../vars_in_z3.h"
#include "../../../utils/utils.h"

#ifndef NDEBUG

#include "../../../parser/ast/expression/constant_expression.h"

#endif

namespace Z3_IN_PLAJA {

    class SmtVarsOf final: public AstVisitorConst {

    private:
        const Z3_IN_PLAJA::StateVarsInZ3* stateVars;
        std::unordered_set<Z3_IN_PLAJA::VarId_type> collectedVars;

        void visit(const Destination* dest) override;
        void visit(const Edge* edge) override;

        void visit(const VariableExpression* exp) override;
        void visit(const ConstantExpression* exp) override;
        void visit(const ConstantArrayAccessExpression* exp) override;

    public:
        SmtVarsOf(const Z3_IN_PLAJA::StateVarsInZ3& state_vars);
        ~SmtVarsOf() override;
        DELETE_CONSTRUCTOR(SmtVarsOf)

        [[nodiscard]] inline std::unordered_set<Z3_IN_PLAJA::VarId_type> retrieve_collected_vars() { return std::move(collectedVars); }

    };

}

/**********************************************************************************************************************/

Z3_IN_PLAJA::SmtVarsOf::SmtVarsOf(const Z3_IN_PLAJA::StateVarsInZ3& state_vars):
    stateVars(&state_vars) {
}

Z3_IN_PLAJA::SmtVarsOf::~SmtVarsOf() = default;

/* */

void Z3_IN_PLAJA::SmtVarsOf::visit(const Destination* dest) {
    if (not stateVars->get_parent().ignore_locs()) { collectedVars.insert(stateVars->loc_to_z3(dest->get_location_index())); }
}

void Z3_IN_PLAJA::SmtVarsOf::visit(const Edge* edge) {
    if (not stateVars->get_parent().ignore_locs()) { collectedVars.insert(stateVars->loc_to_z3(edge->get_location_index())); }
}

void Z3_IN_PLAJA::SmtVarsOf::visit(const VariableExpression* exp) { collectedVars.insert(stateVars->to_z3(exp->get_id())); }

void Z3_IN_PLAJA::SmtVarsOf::visit(const ConstantExpression*) { PLAJA_ABORT } // These should have been optimized out.

void Z3_IN_PLAJA::SmtVarsOf::visit(const ConstantArrayAccessExpression* exp) { collectedVars.insert(stateVars->const_to_z3(exp->get_id())); }


/* */

namespace Z3_IN_PLAJA {

    std::unordered_set<Z3_IN_PLAJA::VarId_type> collect_smt_vars(const AstElement& ast_element, const Z3_IN_PLAJA::StateVarsInZ3& state_vars) {
        SmtVarsOf smt_vars_of(state_vars);
        ast_element.accept(&smt_vars_of);
        return smt_vars_of.retrieve_collected_vars();
    }

}