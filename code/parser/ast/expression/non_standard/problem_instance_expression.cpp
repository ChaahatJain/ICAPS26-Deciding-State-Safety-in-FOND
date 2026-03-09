//
// This file is part of the PlaJA code base.
// Copyright (c) (2019 - 2023) Marcel Vinzent.
// See README.md in the top-level directory for licensing information.
//

#include "problem_instance_expression.h"
#include "../../../visitor/ast_visitor.h"
#include "../../../visitor/ast_visitor_const.h"
#include "../../type/declaration_type.h"
#include "../array_value_expression.h"
#include "predicates_expression.h"

ActionOpTuple::ActionOpTuple(ActionOpID_type op_id, UpdateIndex_type update_index):
    op(op_id)
    , update(update_index) {

}

ActionOpTuple::~ActionOpTuple() = default;

ActionOpTuple::ActionOpTuple(const ActionOpTuple& other):
    op(other.op)
    , update(other.op) {
}

ActionOpTuple::ActionOpTuple(ActionOpTuple&& other): // NOLINT(*-noexcept-move-constructor)
    op(other.op)
    , update(other.op) {
}

void ActionOpTuple::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ActionOpTuple::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

/**********************************************************************************************************************/

ProblemInstanceExpression::ProblemInstanceExpression():
    targetStep(0)
    , policyTargetStep(0)
    , start(nullptr)
    , startPtr(nullptr)
    , reach(nullptr)
    , reachPtr(nullptr)
    , includesInit(false)
    , predicates(nullptr)
    , predicatesPtr(nullptr) {

}

ProblemInstanceExpression::~ProblemInstanceExpression() = default;

/* Static. */

const std::string& ProblemInstanceExpression::get_op_string() {
    static const std::string op_string = "problem-instance";
    return op_string;
}

/* Construction. */

void ProblemInstanceExpression::reserve_op_path() { opPath.reserve(targetStep + 1); }

void ProblemInstanceExpression::add_op_path_step(std::unique_ptr<ActionOpTuple> action_op_tuple) {
    PLAJA_ASSERT(targetStep == 0 ? opPath.empty() : opPath.size() < targetStep)
    opPath.push_back(std::move(action_op_tuple));
}

void ProblemInstanceExpression::reserve_pa_path() { paStatePath.reserve(targetStep + 1); }

void ProblemInstanceExpression::add_pa_state_path_step(std::unique_ptr<ArrayValueExpression>&& pa_state) {
    PLAJA_ASSERT(paStatePath.size() <= targetStep)
    paStatePath.push_back(std::move(pa_state));
}

/* Setter. */

void ProblemInstanceExpression::set_op_path_step(ActionOpID_type op, UpdateIndex_type update, StepIndex_type step) { // NOLINT(*-easily-swappable-parameters)
    PLAJA_ASSERT(step < opPath.size())
    opPath[step]->set_op(op);
    opPath[step]->set_update(update);
}

void ProblemInstanceExpression::set_start(const Expression* start_new) {
    start = start_new;
    startPtr = nullptr;
}

void ProblemInstanceExpression::set_start(std::unique_ptr<Expression>&& start_new) {
    startPtr = std::move(start_new);
    start = startPtr.get();
}

void ProblemInstanceExpression::set_reach(const Expression* reach_new) {
    reach = reach_new;
    reachPtr = nullptr;
}

void ProblemInstanceExpression::set_reach(std::unique_ptr<Expression>&& reach_new) {
    reachPtr = std::move(reach_new);
    reach = reachPtr.get();
}

void ProblemInstanceExpression::set_predicates(const PredicatesExpression* predicates_new) {
    predicates = predicates_new;
    predicatesPtr = nullptr;
}

void ProblemInstanceExpression::set_predicates(std::unique_ptr<PredicatesExpression>&& predicates_new) {
    predicatesPtr = std::move(predicates_new);
    predicates = predicatesPtr.get();
}

void ProblemInstanceExpression::set_pa_state_path_step(std::unique_ptr<ArrayValueExpression>&& pa_state, StepIndex_type step) {
    PLAJA_ASSERT(step < paStatePath.size())
    paStatePath[step] = std::move(pa_state);
}

/* Getter. */

const ActionOpTuple* ProblemInstanceExpression::get_op_path_step(StepIndex_type step) const {
    PLAJA_ASSERT(step < opPath.size());
    return opPath[step].get();
}

const ArrayValueExpression* ProblemInstanceExpression::get_pa_state_path_step(StepIndex_type step) const {
    PLAJA_ASSERT(step < paStatePath.size());
    return paStatePath[step].get();
}

/* Override. */

void ProblemInstanceExpression::accept(AstVisitor* ast_visitor) { ast_visitor->visit(this); }

void ProblemInstanceExpression::accept(AstVisitorConst* ast_visitor) const { ast_visitor->visit(this); }

std::unique_ptr<PropertyExpression> ProblemInstanceExpression::deepCopy_PropExp() const { return deep_copy(); }

/**/

std::unique_ptr<ProblemInstanceExpression> ProblemInstanceExpression::deep_copy() const {
    std::unique_ptr<ProblemInstanceExpression> copy(new ProblemInstanceExpression());
    if (resultType) { copy->resultType = resultType->deep_copy_decl_type(); }

    copy->set_target_step(targetStep);
    copy->set_policy_target_step(policyTargetStep);

    copy->opPath.reserve(opPath.size());
    for (const auto& op: opPath) { copy->add_op_path_step(*op); }

    if (startPtr) { copy->set_start(startPtr->deepCopy_Exp()); }
    else { copy->set_start(start); }

    if (reachPtr) { copy->set_reach(reachPtr->deepCopy_Exp()); }
    else { copy->set_reach(reach); }

    copy->set_includes_init(includesInit);

    if (predicatesPtr) { copy->set_predicates(predicatesPtr->deepCopy()); }
    else { copy->set_predicates(predicates); }

    copy->paStatePath.reserve(paStatePath.size());
    for (const auto& pa_state: paStatePath) { copy->add_pa_state_path_step(pa_state->deep_copy()); }

    return copy;
}
