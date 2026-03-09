//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2020) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "predicate_abstraction_expression.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/declaration_type.h"
#include "objective_expression.h"
#include "predicates_expression.h"

/* Extern. */

namespace PLAJA_UTILS { extern bool file_exists(const std::string& file_name); }

/* Static. */

const std::string& PAExpression::get_op_string() {
    static const std::string op_string = "PA";
    return op_string;
}

/**/

PAExpression::PAExpression() = default;

PAExpression::~PAExpression() = default;

/* Setter. */

void PAExpression::set_objective(std::unique_ptr<ObjectiveExpression>&& objective_r) { objective = std::move(objective_r); }

void PAExpression::set_goal_objective(std::unique_ptr<Expression>&& goal_objective) {
    objective = std::make_unique<ObjectiveExpression>();
    objective->set_goal(std::move(goal_objective));
}

void PAExpression::set_predicates(std::unique_ptr<PredicatesExpression>&& predicates_r) { predicates = std::move(predicates_r); }

/* Getter. */

const Expression* PAExpression::get_objective_goal() const { return objective ? objective->get_goal() : nullptr; }

/* Override. */

void PAExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void PAExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> PAExpression::deepCopy_PropExp() const { return deep_copy(); }

/* */

std::unique_ptr<PAExpression> PAExpression::deep_copy() const {
    std::unique_ptr<PAExpression> copy(new PAExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }
    // misc:
    if (start) { copy->set_start(start->deepCopy_Exp()); }
    if (reach) { copy->set_reach(reach->deepCopy_Exp()); }
    if (objective) { copy->set_objective(objective->deep_copy()); }
    copy->nnFile = nnFile;
    if (predicates) { copy->set_predicates(predicates->deepCopy()); }
    return copy;
}

bool PAExpression::nn_file_exists() const { return PLAJA_UTILS::file_exists(get_nnFile()); }

